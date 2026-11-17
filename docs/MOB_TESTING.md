# MOB testing: how the watch agent and the EssentialSail agent test and debug together

For the agent working on **EssentialSail** (Android app, repository `EssentialSailNmeaV1_57AllVersions`) and the agent working on the
**EssentialWatch** firmware (repository `essentialwatch`, this one). Both run on the same Windows PC of the owner, who relays messages
between the two agents. The owner speaks Italian: reports to the owner are in Italian, files and code in English.

Written 17 Nov 2026 for watch firmware **2.8.0-s3** and the protocol document
`Documentation/MOB_WATCH_PROTOCOL_SPEC.md` of EssentialSail (commit `848fe39`, 971 lines). That document is the **contract**: if the two sides
disagree, the spec wins, and whoever finds a hole writes it down (section 8) instead of guessing.

Read this first, then the spec sections 3 to 9.

## 1. What is being tested

```
   WATCH (ESP32-S3, firmware)                                  TABLET (EssentialSail for Android)
   mob.h   $MOB (A/P/C/N), $MOA check, $MOI/$MOK   <== UDP 10120 ==>   MobMessageListener, WatchMobProtocol
   pair.h  $MOP / $MOQ (6-digit code, ECDH)         <== UDP broadcast ==> WatchPairing.kt
   gps.h   optional GPS module (NMEA) -> lat/lon in $MOB
```

The watch has **no GPS and no compass**: `$MOB` carries an empty position (the tablet then uses the boat position) unless a GPS module is wired.
The siren is the **vibration motor** (GPIO18), there is no sound. Wi-Fi UDP only; Meshtastic is not implemented on the watch.

## 2. Test levels (do them in this order)

| Level | What | Who | Status on 17 Nov 2026 |
|---|---|---|---|
| L0 | Unit tests and the spec vectors (section 13 of the spec) | each side | Tablet: done (its unit tests). Watch: done over USB (`CMOBVEC`, `CMOIVEC`, `CPAIRVEC`): the lines are identical to the spec |
| L1 | Watch against a **fake tablet** written from the spec (`tools/mob_test/`) | watch agent | Done: pairing, alive, alarm, retries, cancel, `$MOI`/`$MOK`, siren, GPS fields |
| L2 | Tablet against a **simulated watch** (`tools/mob_watch_test/send_mob.py` of the EssentialSail repository, `--pair` for the code) | tablet agent | Done for most; `--pair` not yet run against the app (spec 8) |
| L3 | **Real watch against the app on the emulator** `Claude_Test` | both, owner present | **Not done**: needs the network trick of section 4 |
| L4 | **Real watch against the real tablet** | owner + both | Not done. Touch the physical tablet only when the owner asks |
| L5 | Real conditions: boat Wi-Fi, several watches, tablet restart, Wi-Fi loss | owner | Not done |

Do not skip to L3/L4 before L1 and L2 pass: a failure there is then a real interoperability problem and not a bug of one side.

## 3. Shared resources: who may touch what

| Resource | Rule |
|---|---|
| **COM5** (watch USB) | One program at a time. The watch simulator (`tools/watch_sim`, tab Tools) and `watch_usb.py` both open it. The watch agent flashes and drives it. The tablet agent does **not** open COM5 without asking the owner. The simulator has a button "Stop other simulators". |
| Opening COM5 **resets the watch** | The Wi-Fi needs about 10 s to come back. `watch_usb.py` waits 12 s. Put all commands of one session in one call. |
| Watch IP | Normally 192.168.2.x on the network SophosAida (DHCP, it changes). The PC must be on the **same network**, otherwise nothing reaches the watch (the PC was once on 192.168.1.x). Check with `ipconfig` and `python tools/mob_test/watch_usb.py status`. |
| PC firewall | The first time, Windows asks to allow Python on the private network: allow it, otherwise the fake tablet receives nothing. |
| UDP/TCP **10120** | One listener at a time on the PC: the fake tablet, the emulator redirection or the simulator. Check `netstat -ano | findstr 10120` before starting one. |
| Emulator | Only `Claude_Test` (port 5580). Never `adb emu kill`, never the owner's emulator or physical tablet unless asked. |
| Processes | Whoever starts a fake tablet or a simulator stops it when the test ends. Never kill the owner's windows. |
| Watch configuration | After a test the watch agent restores it (MOB off, no id, no key). The owner's own settings (Wi-Fi, NMEA server, web password) are never changed. |

## 4. Making the real watch reach the app (L3)

The emulator lives on a private network (10.0.2.x, the spec example uses `host=10.0.2.16`): the watch cannot reach it directly.

1. **Preferred: the real tablet** on the same Wi-Fi as the watch (L4). Settings > MOB Messages > *Listen for MOB messages* ON. The card must say
   `Listening on UDP + TCP 10120`.
2. **Emulator with redirection** (to be tried, not yet verified): forward the PC port 10120 (UDP and TCP) to the emulator with the emulator console
   (`adb -s emulator-5580 emu redir add udp:10120:10120`, same for `tcp:`), then the watch talks to the **PC address** on port 10120. The tablet
   agent checks that replies (`$MOA`, `$MOQ`, `$MOI`) really go back to the watch and that **broadcast** `$MOP` (pairing) is delivered to the app: UDP
   broadcast to `255.255.255.255:10120` does *not* pass through a host redirection, so for pairing on the emulator the watch must be given the PC address
   (set "MOB server IP" first: then `$MOP` is sent straight to it).
3. If neither works, stay at L1/L2 and say so in the report. Do not declare L3 done.

## 5. Watch-side tools (this repository)

| Tool | What |
|---|---|
| `tools/mob_test/watch_usb.py` | Sends commands to the watch over USB (`status`, `CPAIR`, `CMOBRAISE`, `CSET`, ...). See its header. |
| `tools/mob_test/fake_tablet.py <id> <key64hex> <seconds>` | A tablet from the spec: checks `$MOB`, sends `$MOA`, and after the first `A` sends `$MOI` (own counter, siren on for messages 2 to 4, one duplicate) and checks every `$MOK`. Prints one line per event. |
| `tools/mob_test/fake_pair_tablet.py <code> <seconds> [id]` | A pairing tablet: accepts `$MOP` while the code is right, answers `$MOQ` (ECDH P-256 + HKDF), then validates `$MOB` signed with the derived key. Needs `pip install cryptography`. Counts wrong codes like the app (silent). |
| `tools/watch_sim/` | The boat simulator (NMEA/AIS) for the watch; not needed for MOB. |

Watch USB commands (prefix `C`; the answer is one line):

| Command | Answer / effect |
|---|---|
| `CSTATUS` | JSON: `mob` (off/idle/alarm/cancelling), `mobok` (configured and Wi-Fi up), `mobsent`, `mobacks`, `mobseq`, `mobackage`; `moi` (age in s of the last `$MOI`, absent if none), `moidist`, `moibrg`, `moisiren`, `moisirenon`, `moiacc`, `moidrop`; `gps`, `gpsfix`, `gpslat`, `gpslon`, `gpsage`, `gpssats`, `gpslines`, `gpsbad` |
| `CGET` | Configuration JSON: `mobon`, `mobhost`, `mobport`, `mobid`, `mobkey_set` (the key is never returned), `mobowner`, `pair` (idle/busy/ok/fail), `gpsrx`, `gpsbaud` |
| `CSET {...}` | Sets `mobon`, `mobhost`, `mobport`, `mobid` (4-8 A-Z0-9), `mobkey` (64 hex), `mobowner`, `gpsrx`, `gpsbaud`. Empty `mobkey`/`mobid` clears them. |
| `CMOBVEC` | Prints `$MOB` lines for vectors 5, 9, 10 of the spec (key 00..1f, id W7K2). Must be identical to the spec. |
| `CMOIVEC` | Parses the `$MOI` lines of 13.3 (accept/reject) and prints the `$MOK` for 0008 (`$MOK,W7K2,0008,a23a0c05*23`). |
| `CPAIRVEC` | The pairing example of 8.5: `$MOP` line, accepted reply (key `6f8ba41f7d0b6258d93c1e33ca7b3182b5ebe852fc5e969521bb3cf460ed6fd0`), damaged reply and wrong code rejected. |
| `CPAIR 482913` | Starts pairing (the code may contain a space). Result in `CGET` field `pair`. |
| `CMOBRAISE` / `CMOBCANCEL` | Raise / cancel the MOB without the on-screen confirmation (the screen uses a long press, and a second one to confirm the cancel). |
| `CGPSFEED <nmea sentence with checksum>` | Feeds one NMEA sentence to the GPS parser (test without a module). |

The watch screen: page **MOB** (last page, only when `mobon`). Hold to raise; red circle with the arrow to the boat (north up) and metres/degrees
after a `$MOI`; "Delivering... / Delivered / NOT DELIVERED"; "SIREN - tap to mute".

## 6. Scenario matrix

For each scenario: do it, compare, and write the result in the report (section 7). Columns: expected on the **watch** / on the **tablet**.
"F" = already passed against the watch agent's fake tablet; "A" = to be passed against the real app (tablet agent, then L3/L4).

| # | Scenario | Watch | Tablet | F | A |
|---|---|---|---|---|---|
| 1 | Pair with the right code | `pair`=ok in 1-5 s, `mobid`/`mobhost`/`mobport` set, `mobon` true, key set; first `$MOB` accepted | window closes, device in the list, "Paired" | yes | |
| 2 | Pair with a wrong code | `pair`=fail after 5 s, nothing stored; 4 `$MOP` sent, 1.5 s apart | counts 4 failures, **silent**; one more failure is still allowed | yes | |
| 3 | Pair after the 120 s window | fail after 5 s | `$MOP` ignored silently | | |
| 4 | Pairing with the address typed first ("MOB server IP") | `$MOP` goes to that address only | accepted if the window is open | | |
| 5 | Alive | `P` every 5.5 s, `N` (owner name) first and every 60 s; seq +1 each | "last message" time updates in Settings | yes | |
| 6 | Alarm, tablet answers | `N` then `A`; `A` every 1 s with the **same seq and the same line** until the first `$MOA`, then every 5.5 s with a new seq; page "Delivered" | red banner, beep, widget with the owner name, logbook line | yes | |
| 7 | Alarm, tablet silent (stop the listener) | `A` repeated every 1 s with the same seq; "NOT DELIVERED" after 10 s; keeps trying | nothing | | |
| 8 | Cancel on the wrist | two long presses; `C` every 1 s until `$MOA`, then back to `P` | "The watch asks to cancel the MOB"; the alarm stays until *Mark Safe* | yes | |
| 9 | Mark Safe while the watch keeps sending `A` | keeps sending `A` (the tablet cannot tell it) | acknowledges but ignores them (spec 9.5); `P`/`C` ends the suppression | | |
| 10 | `$MOI` every 5 s | accepts, `$MOK` back in under 100 ms, distance and arrow shown, grey after 15 s without one | "Watch received" | yes | |
| 11 | Ring watch (`siren`=1) | vibration pulses 0.8 s on / 0.4 s off, screen on, MOB page; tap mutes 20 s; stops with `siren`=0 or 12 s without a request | status line, 60 s auto-stop | yes (siren state only, not the motor) | |
| 12 | Duplicate `$MOI` | second `$MOK`, counted once | | yes | |
| 13 | Tablet restarts during a MOB | accepts a `$MOI` with a lower seq after 60 s of silence (**deviation from the spec**, see section 8) | counters restart, MOB lost | | |
| 14 | Wi-Fi lost and back during an alarm | alarm stays raised, sent again as soon as the Wi-Fi is back | | | |
| 15 | GPS module (or `CGPSFEED`) | `A`/`P`/`C` carry `lat,lon,fixage` (5 decimals, age grows); `N` never | position at the person, not the boat | yes | |
| 16 | No GPS | empty `lat,lon,fixage` | uses the boat position for `A` | yes | |
| 17 | Wrong key or id on the watch | `$MOA` never valid, "NOT DELIVERED" | rejects silently; **20 bad messages in a minute block the watch IP for 5 minutes** (an unacked `A` goes out once per second: 20 s are enough) | | |
| 18 | Tablet IP changes (DHCP) | no acks; set the new address (web page, menu, or pair again) | | | |

## 7. How the two agents coordinate

The owner relays. To keep the loop short:

1. **Announce before you touch a shared resource** (COM5, port 10120, the watch configuration, the emulator): one line to the owner ("I am flashing the
   watch", "I am starting the listener on 10120", "I am done, COM5 is free").
2. **Pin the versions** in every report: watch firmware (`CSTATUS` field `fw`), app version and commit, spec commit.
3. **One scenario at a time** from section 6. Same numbers on both sides so the reports match.
4. **Report format** (copy it):
   ```
   MOB test report - <date> - watch fw <x> - app <version/commit> - spec <commit>
   Level: L1 / L2 / L3 / L4        Scenario: <number and name>
   Done:     <what was done, which commands / buttons>
   Expected: <from the matrix>
   Observed: <what happened, with the exact log lines, status JSON, or screenshot>
   Result:   PASS / FAIL / NOT TESTED (why)
   Suspect:  watch / app / spec / network / unknown, and why
   ```
5. **A failure is never "fixed" on the other side's repository.** Each agent changes only its own code. If the cause is in the other side, write the
   evidence (the exact lines on the wire) and hand it over through the owner.
6. **Wire evidence beats opinions.** For a signature problem, compare the exact line with the spec vector and with `send_mob.py` / the watch tool
   output. For a timing problem, give the times of the lines (`fake_tablet.py` prints seconds since the start).
7. **Do not announce "done"** for L3/L4 from L1/L2 results.

## 8. Known points to check, deviations and spec gaps (as of 17 Nov 2026)

Open questions that touch the app or the spec (the tablet agent decides what to change; the watch follows the spec):

1. **`$MOI` sequence after a tablet restart.** The tablet counter for `$MOI` is in memory; with the spec's window rule a watch that remembers a high
   number would drop everything, siren included, until the counter catches up. The watch accepts any sequence number after **60 s** without a `$MOI`
   (a deviation, chosen for safety). Suggestion: put it in the spec (or start the counter at a random value).
2. **Section 9.1 / section 5 / 7.1** do not list `$MOK`, `$MOP`, `$MOQ` as lines the tablet accepts (the code does handle them). Documentation gap only.
3. **Sections 1 and 2** describe the 6-digit pairing as the normal flow; today the tablet's "Pair a watch" shows only the code, and the pairing **link**
   exists only from the "Pairing link" button of an already paired device. The watch accepts both (code, or id + key typed/pasted).
4. **Pairing attempts.** The watch sends the same `$MOP` 4 times in 5 s (one per attempt, alternating `255.255.255.255` and the network's directed
   broadcast, or the typed address). A wrong code therefore uses 4 of the 5 allowed failures. If the app counts every datagram as a failure this is the
   most it can be; tell the watch agent if a different pacing is wanted.
5. **Broadcast on Android.** The app must receive UDP broadcast on port 10120 (`$MOP`). Check it on a real device, not only with loopback tests.
6. **`$MOB` retransmission** repeats the **identical line** (same position, same battery) until the first `$MOA`; `N` is sent before `A` and uses its own
   sequence number (so `A` is `N`+1). Check that the app's replay window accepts that order.
7. **Rate limit vs retries.** With a wrong key an unacknowledged `A` goes out every second, so the 20-per-minute limit (spec 9.6) is reached in 20 s and
   the watch IP is blocked for 5 minutes. A real MOB with a correct key is not affected; test scenario 17 to see the behaviour.
8. **No message tells the watch that the crew pressed Mark Safe** (the watch only sees the `$MOI` stop and keeps sending `A` until cancelled on the wrist).
9. **Not tested at all:** the physical vibration of the siren (only the state), the GPS with a real module, several watches paired to one tablet, the
   Meshtastic gateway (the watch has no Meshtastic code), the NMEA-stream path, the pairing link typed on the watch with the real app.

## 9. Debug checklist

| Symptom | First checks |
|---|---|
| Watch never gets an ack | `CSTATUS`: `mobok` true? (Wi-Fi up, host/id/key set). `mobsent` growing? Is the listener ON and on the same network? Firewall? Is the id the one the tablet shows? |
| `mobsent` grows, tablet shows nothing | Wrong key or id (silent drop), blocked source IP (scenario 17), old sequence after a tablet restart (re-pair or restart both) |
| Pairing "fail" | The code on the tablet is 120 s old or was typed wrong; the tablet is not on the same network; broadcast filtered by the router/AP client isolation: set "MOB server IP" and pair again |
| `$MOI` not shown on the watch | `moidrop` growing in `CSTATUS`: wrong id, wrong signature, old seq, line over 120 bytes. `moiacc` growing but nothing on screen: the MOB page must be the shown page |
| `$MOK` not seen by the tablet | The watch answers to the **source address and port of the `$MOI`**. If the app sends from a different socket than the one that received the `$MOB`, the reply goes there anyway; check what the app expects |
| Siren does not stop | 12 s without `siren`=1 stops it; `siren`=0 stops it at once; a tap mutes for 20 s |
| Watch web page does not answer | It serves one client at a time: close other browser tabs on it |
| COM5 busy | A simulator or another script holds it; ask the owner, do not kill it |
| Everything stopped after a flash | The watch restarts: wait for the Wi-Fi (about 10 s) |
