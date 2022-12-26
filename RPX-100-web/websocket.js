ws = require('ws')

const wss = new ws.WebSocketServer({
    port: 8085,
    perMessageDeflate: {
        zlibDeflateOptions: {
            // See zlib defaults.
            chunkSize: 1024,
            memLevel: 7,
            level: 3
        },
        zlibInflateOptions: {
            chunkSize: 10 * 1024
        },
        // Other options settable:
        clientNoContextTakeover: true, // Defaults to negotiated value.
        serverNoContextTakeover: true, // Defaults to negotiated value.
        serverMaxWindowBits: 10, // Defaults to negotiated value.
        // Below options specified as default values.
        concurrencyLimit: 10, // Limits zlib concurrency for perf.
        threshold: 1024 // Size (in bytes) below which messages
        // should not be compressed if context takeover is disabled.
    }
});

function generateRandomWaterfall(data) {
    for (let i = 0; i < data.span; i++) {
        //data.s.push(-(i % 100));
        data.s.push(-Math.floor(Math.random() * 100))
    }
    return data
}

wss.on('connection', function connection(ws) {
    ws.on('message', function message(data) {
        console.log('received: %s', data.toString("utf-8"));
    });
    const interval = setInterval(function () {
        ws.send(JSON.stringify(generateRandomWaterfall({s: [], center: 7_01_000, span: 2_000, txFreq: 7_123_000})));
    }, 100);
    ws.onclose = () => clearInterval(interval);
});
