# EssentialWatch simulator

A pretend boat for the watch: an NMEA 0183 TCP server (own boat + AIS targets) with scenarios made for the watch pages,
plus a control panel for the real watch (its IP address, live screen copy, keys, status, vibration test).

Python 3.9+ with Tkinter; Pillow optional (live screen copy). Run `python essentialwatch_sim.py` (Windows: `EssentialWatch_Sim.bat`),
press **Start**, then in the watch web page set the NMEA server to the IP and port shown. The watch IP appears when it connects.
Headless: `python essentialwatch_sim.py --headless --scenario corsica --port 10112 --seconds 600`.

| Scenario | What it does |
|---|---|
| Cadaques coastal | 0.3 nm off the coast, Tramontana 28 kn, steep seabed 18-40 m, 4 vessels |
| 12 nm off Corsica | Mistral 22 kn, deep water, 4 vessels, a collision course every 150 s |
| L'Espiguette shallow shelf | depth 5 m down to 1.3 m: minimum depth alarm |
| Strong wind and drift | 40 kn gusting 50, the wind turns round in 8 minutes, the boat yaws |
| Drill: collision | a collision course (CPA 0, TCPA 8 min) every 90 s: flashing frame, AIS page, BOOT snooze |
| Drill: shallow water | depth 8 m to 1 m and back every 2 minutes |
| Drill: signal lost | data 25 s, silence 20 s: the "SEGNALE PERSO" watermark |

Controls: wind direction / speed, depth (or from the scenario), collision target now, harmless target, remove targets,
signal lost. The AIS table shows distance, bearing, CPA and TCPA, in red the targets the watch shows in red.
Watch buttons: Right, Left, Double Tap, Power, Boot, Test vibration; the admin password of the watch is needed (HTTP Digest login).

The scenarios are inspired by the EssentialSail simulator and written from scratch. Depth and positions are invented: never use them for navigation.
