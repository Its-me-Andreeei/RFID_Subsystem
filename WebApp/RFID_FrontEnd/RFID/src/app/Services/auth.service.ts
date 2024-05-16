import {Injectable} from "@angular/core";
import {HttpClient} from "@angular/common/http";
import {Observable} from "rxjs";
import {Admin} from "../Models/Admin";
import {HashService} from "./hash.service";

@Injectable({
  providedIn: 'root'
})

export class AuthService {
  private pathToLogin: string = "http://localhost:8080/login";
  private pathToRegister: string = "http://localhost:8080/register";
  private http: HttpClient;

  loginStorage :Storage = <Storage>sessionStorage;

  constructor(http: HttpClient, private hashService: HashService) {
    this.http = http;
  }

  verifyCredentials(admin: Admin): Observable<any> {
    if(admin.name != null && admin.password != null) {
      admin.password = this.hashService.hashPassword(admin.password, admin.password);
    }
    return this.http.post<Admin>(this.pathToLogin, admin);
  }

  setUserLoggedInStatus(status: boolean, name: string) {
    if(status)
    {
      this.loginStorage.setItem("username", JSON.stringify(name));
      this.loginStorage.setItem("isLoggedIn", JSON.stringify(true));
    }
    else
    {
      this.loginStorage.setItem("isLoggedIn", JSON.stringify(false));
      this.loginStorage.setItem("username", "");
    }
  }

  getLoginStatus() : string | null
  {
    return this.loginStorage.getItem("isLoggedIn");
  }

  getUsername()
  {
    const user = this.loginStorage.getItem("username");
    if(user != null)
    {
      return user.replace(/"/g, '');
    }
    return null;
  }

  logOff() : void{
    this.loginStorage.setItem("username", "");
    this.loginStorage.setItem("isLoggedIn", "false");
  }

  registerUser(admin: Admin) :Observable<any>{
    if(admin.name != null && admin.password != null) {
      admin.password = this.hashService.hashPassword(admin.password, admin.password);
    }
    return this.http.post<Admin>(this.pathToRegister, admin);
  }

}
