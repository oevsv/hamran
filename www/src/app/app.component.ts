import { Component } from '@angular/core';
import {callsign, name} from './global-variables';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent {
  title = name + "-" + callsign;
}
