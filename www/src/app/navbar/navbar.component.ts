import { Component } from '@angular/core';
import {WebsocketService} from "../websocket.service";
import {Meta} from "@angular/platform-browser";

@Component({
  selector: 'app-navbar',
  templateUrl: './navbar.component.html',
  styleUrls: ['./navbar.component.scss']
})
export class NavbarComponent {

  channels = [
    {'name': 'APRS', 'id':  4},
    {'name': 'ADL315', 'id':  5},
    {'name': 'Hermannskogel', 'id':  6},
    {'name': 'Sandl', 'id':  7},
    {'name': 'Troppberg', 'id':  8},
    {'name': 'Hochstrass', 'id':  9},
    {'name': 'R0', 'id':  10},
    {'name': 'R1', 'id':  11},
    {'name': 'R2', 'id':  12},
    {'name': 'R3', 'id':  13},
    {'name': 'R4', 'id':  14},
    {'name': 'R5', 'id':  15},
    {'name': 'R6', 'id':  16},
    {'name': 'R7', 'id':  17},
    {'name': 'R60', 'id':  18},
    {'name': 'R68', 'id':  19}
  ]

  bands = [
    {name: "6m: 50-54 MHz", id: 1},
    {name: "2m: 144-146 MHz", id: 2},
    {name: "70cm: 430-440 MHz", id: 3},
  ]
  bandwidth = [
    {"name": "1 MHz", value: 1},
    {"name": "2 MHz", value: 2},
    {"name": "4 Mhz", value: 4},
    {"name": "8 Mhz", value: 8},
    {"name": "16 Mhz", value: 16}
  ]
  modes = [
    {"name": "WRAN (IEEE 802.22)", id: "WRAN" },
    {"name": "FSK", id: "FSK" }
  ];

  rfPower = [
    {"name": "20dBm", value: 20 },
    {"name": "27dBm", value: 27 },
    {"name": "30dBm", value: 30 },
    {"name": "37dBm", value: 37 },
    {"name": "40dBm", value: 40 },
    {"name": "44dBm", value: 44 }
  ];

  txBeacon = [
    {"name": "One Time", value: -1 },
    {"name": "5min", value: 5 },
    {"name": "30min", value: 30 },
    {"name": "60min", value: 60 }
  ];


  constructor(private websocket: WebsocketService, private meta: Meta) {
  }

  public cmd(key: string, value: any) {
    this.websocket.cmd(key, value);
  }

  selectBand(id: number) {
    this.cmd('band', id)
  }

  selectBandWidth(bandwidth: any) {
    this.cmd('bandwidth', bandwidth)
  }

  getCallSign() {
    const callsign: HTMLMetaElement|null = this.meta.getTag("name=callsign");
    if (callsign) {
      return callsign.content
    } else {
      return "OE3BIA"
    }
  }

  selectChannel(id: number) {
    this.cmd("channel", id)
  }


  selectMode(id: string) {
    this.cmd("mode", id)
  }
  selectRFPower(id: number) {
    console.log("not implemented: selectRFPower");
    return;
  }
  selectTxBeacon(id: number) {
    console.log("not implemented: selectTxBeacon");
    return;
  }
}
