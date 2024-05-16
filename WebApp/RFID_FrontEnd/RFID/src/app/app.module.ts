import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { AdminLoginComponent } from './admin-login/admin-login.component';
import {HttpClientModule} from "@angular/common/http";
import {BrowserAnimationsModule} from "@angular/platform-browser/animations";
import {FormsModule, ReactiveFormsModule} from "@angular/forms";
import {MatToolbarModule} from "@angular/material/toolbar";
import {MatIconModule} from "@angular/material/icon";
import {MatButtonModule} from "@angular/material/button";
import {MatDialogModule} from "@angular/material/dialog";
import {MatFormFieldModule} from "@angular/material/form-field";
import {MatInputModule} from "@angular/material/input";
import {MatTableModule} from "@angular/material/table";
import {MatPaginatorModule} from "@angular/material/paginator";
import {MatSortModule} from "@angular/material/sort";
import {MatSelectModule} from "@angular/material/select";
import {MatCheckboxModule} from "@angular/material/checkbox";
import { RoomManagerComponent } from './room-manager/room-manager.component';
import { RegisterComponent } from './register/register.component';
import {MatTooltip} from "@angular/material/tooltip";
import {MatBadge} from "@angular/material/badge";
import {MatDivider} from "@angular/material/divider";
import {MatCard, MatCardContent, MatCardHeader} from "@angular/material/card";
import {MatStep, MatStepper} from "@angular/material/stepper";

@NgModule({
  declarations: [
    AppComponent,
    AdminLoginComponent,
    RoomManagerComponent,
    RegisterComponent
  ],
  imports: [
    BrowserModule,
    AppRoutingModule,
    BrowserAnimationsModule,
    MatToolbarModule,
    MatIconModule,
    MatButtonModule,
    MatDialogModule,
    MatFormFieldModule,
    MatInputModule,
    MatCheckboxModule,
    ReactiveFormsModule,
    HttpClientModule,
    MatTableModule,
    MatPaginatorModule,
    MatSortModule,
    MatSelectModule,
    FormsModule,
    MatTooltip,
    MatBadge,
    MatDivider,
    MatCard,
    MatCardHeader,
    MatCardContent,
    MatStepper,
    MatStep
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
