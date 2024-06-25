import { Component } from '@angular/core';
import {Admin} from "../Models/Admin";
import {AuthService} from "../Services/auth.service";
import {Router} from "@angular/router";

@Component({
  selector: 'app-register',
  templateUrl: './register.component.html',
  styleUrl: './register.component.css'
})
export class RegisterComponent {
  admin: Admin;
  hide: boolean = true;
  confirmedPassword: string;

  constructor(private authService: AuthService, private router: Router) {
    this.admin = {};
    this.confirmedPassword = ""
  }


  onSubmit() {
    if(this.confirmedPassword !== "" && this.admin.name !== "" && this.admin.password !==""){
      if(this.admin.password === this.confirmedPassword)
      {
        this.authService.registerUser(this.admin).subscribe(
          response =>{
            if(response === 'CREATED')
            {
              alert("Thank your for registration");
              this.router.navigate(['login']);
            }
            else
            {
              alert("[ERROR] User was not registered");
            }
          }
        )
      }
      else
      {
        alert("Passwords do not match");
      }
    }
  }
}
