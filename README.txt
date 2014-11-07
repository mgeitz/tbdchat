Client Features:
    - Login
        Prompt between primary execution loop and server connect can handle a users login
      
    - Register
        Prompt between primary execution loop and server connect can handle new user registration
      
    - Password
        User enters a password for registration and login
      
Server Features:
    - Register
        Allows a single registration for any given unique username 
        Stores a password associated with each username
        Saves username/realname/password to a file so registration persists after server is shutdown
     
Known Bugs / Errors:
    - Server - Can have a conversation between the same user logged in two times
             
             -No way of safely exiting server.  Currently CTRL-C is the only method,
              which closes the server socket, but does not close any client sockets
              or free the memory used for the user list.  Major problem.

             -If client exits during registration, server still completes login process and 
              can start a conversation with a user that does not exist

    - Client/Server - Certain input allows client to continue past registration without 
                      waiting for another user, which starts a conversation with "INVALID"

                    -Passwords aren't working 100% of the time

Future Plans:
    - Visual - Use Curses library to take over terminal window.  Plan to have the top portion of the
               screen show conversation and bottom allow user to input message

    - Functional - Redo registration/login process to be less confusing/make it easier to support multiple
                   users per conversation

                 - Allow conversations of more than two people

                 - Allow users to join/leave conversations in real time (not sure how this would work with logging)

                 - Encrypt passwords client-side

                 - Possibly encrypt all messages between users but this would require some kind of key generation/exchange
                   which could potentially be a pain.     
      
