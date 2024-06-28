# hydrogen

Initially, this works very much like Moonraker, providing a WebSocket proxy to the Klipper API. Later, this will also be the component that replaces Klipper itself, but that's going to be a little bit further down the road. 

This is a C program that is essentially a JSON queue manager. Most of what this program does involves moving elements around in different multithreaded and thread-safe queues. Even logging activities use a queue - the first queue created in fact. 

Each queue has its own queue manager that determines what happens when something arrives in its queue. Logging, for example, will look at the queued element and either print it to the console, write it out to a log file, or add it to the database. This is handled separately from whatever generated the log event so *that* process doesn't have to wait for any of these actions to complete before continuing on. 

