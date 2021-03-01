function UDPServer(port) {
    this.nativeIndex = udpserver_create_native(port);
}

UDPServer.prototype.start = function() {
    udpserver_start_native(this.nativeIndex);
};
