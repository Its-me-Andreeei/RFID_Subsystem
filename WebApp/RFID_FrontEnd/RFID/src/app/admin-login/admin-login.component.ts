import {Component, OnInit} from '@angular/core';
import {AuthService} from "../Services/auth.service";
import {Admin} from "../Models/Admin";
import {Router} from "@angular/router";

@Component({
  selector: 'app-admin-login',
  templateUrl: './admin-login.component.html',
  styleUrl: './admin-login.component.css'
})
export class AdminLoginComponent implements OnInit {
  admin: Admin;
  tmp_user: Admin;
  hide: boolean = true;

  constructor(private authService: AuthService, private router: Router) {
    this.admin = {};
    this.tmp_user = {};
  }

  ngOnInit(): void {
    if(this.authService.getLoginStatus()==='true')
    {
      this.router.navigate(['home']);
    }
  }


  onSubmit() {
    this.tmp_user.name = this.admin.name;
    this.tmp_user.password = this.admin.password;

    this.admin.password = "";
    this.authService.verifyCredentials(this.tmp_user).subscribe(
      response =>{
        console.log(response);
        if(response === 'OK')
        {
          if (this.tmp_user.name != null) {
            this.authService.setUserLoggedInStatus(true, this.tmp_user.name);
            this.router.navigate(['home']);
          }
          else
          {
            this.authService.setUserLoggedInStatus(false, "");
          }
        }
        else
        {
          alert("Invalid Username or password");
          this.authService.setUserLoggedInStatus(false, "");
        }
      }
    )
  }

  goToRegisteration() {
    this.router.navigate(['register']);
  }
}
