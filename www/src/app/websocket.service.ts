import {EventEmitter, Injectable} from '@angular/core';

@Injectable({
  providedIn: 'root'
})
export class WebsocketService {

  private ws?: WebSocket;
  private emitter: EventEmitter<any> = new EventEmitter();
  private reconnectDesired: boolean = true;

  constructor() {
    this.initializeWebsocket();
  }

  getSocket(): WebSocket {
    return <WebSocket>this.ws;
  }

  getEmitter(): EventEmitter<any> {
    return this.emitter;
  }



  initializeWebsocket() :void {
    const that = this;
    this.connectWS();

    // @ts-ignore
    this.ws.onopen = function() {
      console.log("connected!");
    }
    // @ts-ignore
    this.ws.onclose = () => that.handleCose();


    // @ts-ignore
    this.ws.onerror = function(evt) {
      console.error(evt);
    }
    // @ts-ignore
    this.ws.onmessage = function (evt) {
      that.emitter.emit(JSON.parse(evt.data));
    }
  }

  cmd(c: any, p: any) {
    this.ws?.send(c + ':' + p);
  }

  private handleCose() {
    console.log("closed");
    if (this.reconnectDesired) {
      setTimeout(() => this.initializeWebsocket(), 1000);
    }
  }

  private connectWS(): void {
    if (window.location.host.startsWith('https')) {
      this.ws = new WebSocket("wss://" + window.location.host.substring(0, window.location.host.indexOf(':')) + ":8085");
    } else {
      this.ws = new WebSocket("ws://" + window.location.host.substring(0, window.location.host.indexOf(':')) + ":8085");
    }
  }
}

