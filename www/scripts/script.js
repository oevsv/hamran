'use strict';

let spectrum, ws;

function connectWebSocket(spectrum) {

    if (window.location.host.startsWith('https'))
    {
        ws = new WebSocket("wss://" + window.location.host + "/ws/");
    } else
    {
        ws = new WebSocket("ws://" + window.location.host + "/ws/");
    }
    
    spectrum.setWebSocket(ws);
  
    ws.onopen = function() {
        console.log("connected!");
    }
    ws.onclose = function() {
        console.log("closed");
        setTimeout(function() {
            connectWebSocket(spectrum);
        }, 1000);
    }
    ws.onerror = function(evt) {
        console.log("error: " + evt.message);
    }
    ws.onmessage = function (evt) {
        const data = JSON.parse(evt.data);
        if (data.s) {
            spectrum.addData(data.s);
            let num = parseFloat(data.center);
            let bw = parseFloat(data.span);
            let tx = parseFloat(data.txFreq);
            document.getElementById('center').innerHTML = 'RX: ' + (num/1_000_000).toFixed(3) + ' MHz |';
            document.getElementById('tx').innerHTML = ' TX: ' + (tx/1_000_000).toFixed(3) + ' MHz';
            document.getElementById('min').innerHTML = ((num - bw/2)/1_000_000).toFixed(3) + ' MHz';
            document.getElementById('max').innerHTML = ((num + bw/2)/1_000_000).toFixed(3) + ' MHz';
        } else {
            if (data.center) {
                spectrum.setCenterHz(data.center);
                
            }
            if (data.span) {
                spectrum.setSpanHz(data.span);
            }
            if (data.gain) {
                spectrum.setGain(data.gain);
            }
            if (data.framerate) {
                spectrum.setFps(data.framerate);
            }
            spectrum.log(" > Freq:" + data.center / 1000000 + " MHz | Span: " + data.span / 1000000 + " MHz | Gain: " + data.gain + "dB | Fps: " + data.framerate);
        }
    }
}

function cmd(c, p) {
    let command = c + ':' + p;
    ws.send(command);
}

jQuery(function main() {
    
    // Create spectrum object on canvas with ID "waterfall"
    spectrum = new Spectrum(
        "waterfall", {
            spectrumPercent: 25,
            logger: 'log',            
    });

    // Connect to websocket
    connectWebSocket(spectrum);

    // Bind keypress handler
    window.addEventListener("keydown", function (e) {
        spectrum.onKeypress(e);
    });

    jQuery('#ddChannel a[data-command-name], ' +
           '#ddBandWith a[data-command-name], ' +
           '#ddBandSelect a[data-command-name], ' +
           '#ddMode a[data-command-name], ' +
           '#ddPower a[data-command-name], ' +
           '#ddTxBeacon a[data-command-name]')
        .on('click', function (e) {
            e.preventDefault();
            const t = e.target;
            cmd(t.dataset.commandName, t.dataset.commandParam);
    })
});
