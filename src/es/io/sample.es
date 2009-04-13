Sockets
----------------------------------------------------------------
    Given:
        var sock: Socket = new Socket

    Client sockets

        Sync
            sock.connect("www.embedthis.com", 80)
            while (! sock.eof) {
                data = sock.read()
            }
            sock.close()

        Async
            function callback(e: Event) {
                var s: Socket = e.data
                written = s.write(someData, timeout)        // However should not block due to writeable event
            }
            sock.setCallback(WRITE, callback)
            sock.connect("www.embedthis.com", 80)
            return

    Server
        Sync
            var s: Socket = sock.listen("0.0.0.0", 7777)

            data = s.readByte()
            data = s.readString()
            data = s.readVector()
        //  How to deserialize? -- Json
            data = s.readObject()


