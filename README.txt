*   KNOWN BUGS:
*********************************************************
*        
*
*      Server:
*
*       [URGENT]
*         - Users persist in active users and as users in rooms after exiting.
*         - Server seems to freeze entirely when a client uses /join with several actively chatting users present
*
*
*      Client:
*
*       [URGENT]
*         - After resize event, a key must be pressed before input is read into the buffer
*         - after "sitting around a while logged in" and then exiting using /exit or /quit, 
*              exit will hang waiting on chatRX thread
*
*       ["FEATURES"]
*         - userInput is writing special keys to buffer we don't want there
*         - Some special keys print more chars in the input window than backspace removes
*
*       [ALMOST SHOULDN'T CARE]
*         - Since the changes incorporating getch() into userInput, when parsed 
*              by strsep /login and /join return the base commands as args[8], oppose to expected result of args[0].
*            o This does not happen for any other command. This is absolutely dumbfounding.
*            o The packet buffer is read perfectly fine everywhere else
*            o This is absolutely related to how the buffer is now populated
*        
*        
*********************************************************


TO-DO LIST:


   Smaller adjustments:

      - [Client/Server] /show real name, /username real name, etc - return the user name for a give real name (maybe add check to be in users room?)

      - [Client] /ignore username. Ignore all messages (dont print) all messages from a user (requires /show)

      - [Client/Server] /friends and friends list

      - [Client/Server] /away messages

      - [Server] Send notification to all users in a room when another user leaves it
 
      - [Server] Hash stored passwords, hash input on compare

      - [Client/Server] /logout command (remove from rooms/active users, set logged out, keep connection)

      - [Client/Server] /disconnect command (same as /exit but allows the client to /connect again)

      - [Server/Client] Private / Public room settings, room admins, other room attributes

      - [Server/Client] 2 person rooms, /msg username, /tell username, or /query username

      - [client] Curses
         o Add client window to primary screen to display clients currently in room
         o Add minimum terminal size requirement
         o Improved notification/alert formatting


   Requires considerable time:

      - [Client] Client message archiving; by room name, readable format and easy to parse to read back on client
         o Used logged contents to redraw window after resize event

      - [Client] Add more to config, read entire config on startup
         o Color preferences
         o more stuff? anyone? config stuff?

      - [Client/Server] Allow clients to be in n rooms at a time

      - [Client/Server] Full encryption using something like openssl or libsodium

      - [Server] Logging 3 levels (root, debug, info); connections, logins, regisrations, disconnects, messages, etc (will solve resize wipe problem)



CLIENT FEATURES:

   Commands:

      - /help
         Description: Display a list of all available commands.
         Usage: /help
         Description: Display usage and description for a command.
         Usage: /help [command]
         Description: Display usage and descriptions for all commands.
         Usage: /help all

      - /exit
         Description: Safely disconnect from the server and end.
         Usage: /exit

      - /quit
         Description: Safely disconnect from the server and end.
         Usage: /quit

      - /register
         Description: Register as a new user.
         Usage: /register [username] [password] [password]

      - /login
         Description: Login as a previously registered user.
         Usage: /login [username] [password]

      - /who
         Description: Print a list the list of users in the same room as you.
         Usage: /who
         Description: Print a list the list of all connected.
         Usage: /who all
         Description: Print a users real name.
         Usage: /who [username]

      - /invite
         Description: Invite a user to your current room.
         Usage: /invite [username]

      - /join
         Description: Requests to join the room of the name given, if it exists join it, otherwise create it and join it.
         Usage: /join [roomname]

      - /setpass
         Description: Change your current users password.
         Usage: /setpass [current_password] [new_password] [new_password]

      - /setname
         Description: Change your displayed name.
         Usage: /setname [newusername]

      - /connect
         Description: Connects the client to a chat server.
         Usage: /connect [address] [port]

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

      - /clear
         Description: clear contents of chat window.
         Usage: /clear


   Features:

      - Supports orderless interaction
      - Configuration file
      - auto-reconnect
      - Pretty awesome curses interface
     
 
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
 

