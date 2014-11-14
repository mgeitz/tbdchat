Client:
   Commands:
      - /help
         Description: Display a list of all available commands.
         Usage: /help

      - /exit
         Description: Inform the server you are quitting, proceed to disconnect and wonder why your client is still running.
         Usage: /exit

      - /register
         Description: Register as a new user.
         Usage: /register username password password

      - /login
         Description: Login as a previously registered user.
         Usage: /login username password

      - /who
         Description: Print a list the list of users you are chatting with.
         Usage: /who

      - /invite
         Description: Invite a user to your current room.
         Usage: /invite username

      - /room
         Description: This shoud probably be /join or /joinroom I think, not sure why I went with /room.
         Usage: /room roomnumber

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

   Features:
      - Supports orderless interaction
     
 
Server:
   Features:
      - Register
         o Allows a single registration for any given unique username 
         o Stores a password associated with each username
         o Saves username/realname/password to a file so registration persists after server is shutdown
      - Other things worth mentioning . . .
    
 
Known Bugs / Errors:
   Server:
      - Can have a conversation between the same user logged in two times
             
      - No way of safely exiting server.  Currently CTRL-C is the only method,
          which closes the server socket, but does not close any client sockets
          or free the memory used for the user list.  Major problem.

   Client:
      - There is a very strange issue regarding username persisting in client after registration that login and setpass somehow do not have.

      - I'm sure there are other bugs

     
Future Plans:
   Visual:
      - Use Curses library to take over terminal window.  Plan to have the top portion of the
         screen show conversation and bottom allow user to input message

   Functional:
      - Implement rooms

      - Allow users to join/leave conversations in real time (not sure how this would work with logging)

      - Full encryption
