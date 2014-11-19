CLIENT FEATURES:

   Commands:
      - /help
         Description: Display a list of all available commands.
         Usage: /help

      - /exit
         Description: Safely disconnect from the server and end.
         Usage: /exit

      - /quit
         Description: Safely disconnect from the server and end.
         Usage: /quit

      - /register
         Description: Register as a new user.
         Usage: /register username password password

      - /login
         Description: Login as a previously registered user.
         Usage: /login username password

      - /who
         Description: Print a list the list of users in the same room as you.
         Usage: /who
         Description: Print a list the list of all connected.
         Usage: /who all
         Description: Print a users real name.
         Usage: /who username

      - /invite
         Description: Invite a user to your current room.
         Usage: /invite username

      - /join
         Description: Requests to join the room of the name given, if it exists join it, otherwise create it and join it.
         Usage: /join roomname

      - /setpass
         Description: Change your current users password.
         Usage: /setpass current_password new_password new_password

      - /setname
         Description: Change your displayed name.
         Usage: /setname newusername

      - /connect
         Description: Connects the client to a chat server.
         Usage: /connect address port

      - /debug
         Description: Toggle debug mode [print out structured content of each packet received].
         Usage: /debug

      - /autoconnect
         Description: Toggle auto-reconnect on startup.
         Usage: /autoconnect

      - /leave
         Description: Leave the room you are current chatting in and return to the lobby.
         Usage: /leave

      - /motd
         Description: Print out the servers message of the day.
         Usage: /motd

      - /list
         Description: Print out a list of public rooms with active users in them.
         Usage: /list

   Features:
      - Supports orderless interaction
      - Configuration file
      - auto-reconnect
     
 
SERVER FEATURES:

   Features:
      - Register
         o Allows a single registration for any given unique username 
         o Stores a password associated with each username
         o Saves username/realname/password to a file so registration persists after server is shutdown
      - Reacts to accordingly to all client commands
      - Chat with n clients at a time
      - Rooms containing unique chat sessions simultaneously
         o Create or join rooms
         o invite others to join your room 
 

KNOWN BUGS / ERRORS:

   Server:
      - Problems with accessing / modifying user lists
         o Users not being removed from any lists after /exit is received from them, sending messages to disconnected sockfds
            
   Client:
      - Only supports being in one room.
      - User input is not sanatized at all; formatting strings and other terribe things can be transmitted in the buffer, could be resolved on the server.


POTENTIAL RACE CONDITIONS:

      - Server:
         o Rooms list [addresses]
         o Registered users list [addresses]
         o Active users list [addressed]
         o Room specific user lists [addresses]

      - Client: 
         o Current room variable access [addressed]
         o Display name buffer access [addressed]
         o Debug variable access [addressed]
         o Configuration file access [addressed]
         o Printing anything while typing on the client (any cli or gui should solve this)
     

TO-DO LIST:

      - Use Curses library to take over terminal window.  Plan to have the top portion of the
         screen show conversation and bottom allow user to input message
      
      - Track which users are permitted to join a particular preexisting room, give rooms state of public or private.
       
      - Private / Public room settings, other room variables
 
      - Assign the creator of a room some unique flagged ability to make the room private, or set other room features
        
      - Full encryption, but we should start with trying to hash the stored passwords
   
      - logout command to put user back in not-logged-in state, disconnect command to close serverfd and chatrxthread but not close client

      - Remove a user from active_users_list and their room list when they send a /exit

      - Logging; added debug and root level logs, give them flags set by /debug, change debug to persist. Add normal logs for each room. I think we should worry about displaying them in chat later, maybe /log toggles active message logging?

      - Probably move the primary execution loop in main of client to its own method again.

      - Notify all in room when someone else in the room leaves (currently sends message to default channel when a user disconnects)
