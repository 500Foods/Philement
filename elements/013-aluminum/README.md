# aluminum
For the typical Home Assistant user, everything important needs to be on the dashboard. And should support as many customizable entities as possible. The [Moonraker HA integration](https://github.com/denkyem/home-assistant-moonraker) works very well in this regard. Here's a screenshot of what it looks like. 
![image](https://github.com/500Foods/Philement/assets/41052272/99b39dee-017f-4d8b-8b1a-c7314a7c1db1)

## Brainstorming
- Easy installation into Home Assistant. HA-Moonraker was no problem at all.
- Connect directly to a hydrogen instance, which itself can support multiple printers
- Create entities from a list that hydrogen provides, organized ideally by printer
- Support for filament management via fluorine
- Try to simplify templating, options for showing 300C vs. 300.0C, that sort of thing
- Maybe custom cards for showing temperature graph or filament inventory
- Custom card for "now printing" inspired by nitrogen, or even directly using it
- Access to log, either Klipper log or hydrogen, also submit to log
- Access to actions (pause, cancel, emergency, exclude objects, etc.) as buttons
- Event handling should easily follow how entities are structured
- Lots of documentation about how to do various things, notifications in particular
- Lots of examples of how to customize dashboards
- Simple dashboards - status/progress
- Full-on dashboards running into the ballpark of nitrogen/lithium/KlipperScreen
- Examples of how to incorporate live nitrogen/lithium interfaces into webpage card
