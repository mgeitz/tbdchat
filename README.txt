Client:
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
     
 
Server:
   Features:
      - Register
         o Allows a single registration for any given unique username 
         o Stores a password associated with each username
         o Saves username/realname/password to a file so registration persists after server is shutdown
      - Chat with n clients at a time
      - Rooms containing unique chat sessions simultaneously
    
 
Known Bugs / Errors:
   Server:
      - Can have a conversation between the same user logged in two times
             
      - No way of safely exiting server.  Currently CTRL-C is the only method,
          which closes the server socket, but does not close any client sockets
          or free the memory used for the user list.  Major problem.

      - Problems with accessing / modifying user lists
         o I think in order to establish private (invite required) rooms we will need to store an additional linked list
            of users with each room node to keep track of who has been invited to the channel. If that list is empty
            then I guess we can assume the channel to be public.

   Client:
      - Only supports being in one room.

   Potential race conditions to still address
:
      - Client: 
         o Printing anything on the client (any cli or gui will solve this)
      - Server:
         o List reading / writing 
     
Future Plans:
   Visual:
      - Use Curses library to take over terminal window.  Plan to have the top portion of the
         screen show conversation and bottom allow user to input message
      
   Functional:
      - Track which users are permitted to join a particular preexisting room.
        
      - Assign the creator of a room some unique flagged ability to make the room private
        
      - Full encryption, but we should start with trying to hash the stored passwords
   
   Things that should be added with rooms:
      - Invite command
        
      - Exit room command
      
