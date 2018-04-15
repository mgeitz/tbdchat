# TBDChat

## Simple NCurses Chat Client/Server Using BSD Sockets

![TBDChat][TBDChat1]
![TBDChat][TBDChat2]

[TBDChat1]: http://i.imgur.com/lGuTIT2.jpg
[TBDChat2]: http://i.imgur.com/2QfQJIA.jpg

### Client Features

- Clean NCurses Interface
- Extensive local help system
- Orderless interaction
- Configuration file
- Auto-reconnect

### Server Features

- Allows a single registration for any given unique username
- Stores a SHA256 hashed password associated with each username
- Saves username/realname/passhash to a file so registration persists after server is shutdown
- Reacts accordingly to all client commands dependent on client state
- Chat with n clients at a time
- Rooms containing unique chat sessions simultaneously
- Create or join rooms
- Invite others to join your room
- Each room supports n clients
- Sanitizes input fields which require so accordingly
- SHA256 hashing for password storage

### Dependencies

#### Debian
```sh
$ apt-get install build-essential libncurses5-dev libssl-dev
```

### Getting Started

#### Docker Client
```sh
$ docker-compose build tbdc
$ docker-compose run --rm tbdc
```

#### Docker Server
```sh
$ docker-compose build tbdc_server
$ docker-compose run --rm tbdc_server
```

#### Client Compilation
```sh
$ make chat_client
```

#### Server Compilation
```sh
$ make chat_server
```

#### Compile All the Things
```sh
$ make all
```

### Contributing
View the section on [how to contribute](./CONTRIBUTING.md)
