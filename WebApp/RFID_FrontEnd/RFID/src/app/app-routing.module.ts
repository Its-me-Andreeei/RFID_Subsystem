import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import {AdminLoginComponent} from "./admin-login/admin-login.component";
import {RoomManagerComponent} from "./room-manager/room-manager.component";
import {RegisterComponent} from "./register/register.component";

const routes: Routes = [
  {
    path: '', redirectTo: 'login', pathMatch: 'full'
  },
  {
    path: 'login', component: AdminLoginComponent
  },
  {
    path: 'home', component: RoomManagerComponent
  },
  {
    path: 'register', component: RegisterComponent
  }
  ];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
