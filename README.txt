
TO-DO LIST:

   Smaller adjustments:

      - Remove a user from active_users_list and their room list when they send a /exit
      
      - Send notification to all users in a room when another user leaves it

      - Private / Public room settings, room admins, other room variables
 
      - Hash stored passwords, hash input on compare
   
      - /logout command (remove from rooms/active users, set logged out, keep connection)

      - /disconnect command (same as /exit but allows the client to /connect again)

      - Send a notification to all users in a room when another user disconnects entirely


   Requires considerable time:

      - Curses

      - Server logging; connections, logins, regisrations, disconnects, messages, etc

      - Client message archiving; by room name, readable format and easy to parse to read back on client 

      - Full encryption using something like openssl or libsodium

      - Implement per-room user list mutexes

      - Allow clients to be in n rooms at a time



KNOWN BUGS:

   Server:

      - Rooms with a space as the name can be created, we should enforce room names of a certain length? May also need to enforce a min length on usernames and realnames, as well as a more reasonable max length on realname.
            
   Client:

      - 



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

      - Sanitizes all processed input from connections
 

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
