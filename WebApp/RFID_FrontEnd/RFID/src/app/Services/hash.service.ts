import { Injectable } from '@angular/core';
import CryptoJS from 'crypto-js';

@Injectable({
  providedIn: 'root'
})
export class HashService {

  constructor() { }

  hashPassword(name: string, password: string): string {
    // Hash the password using SHA-256 algorithm
    return CryptoJS.SHA256(name + password).toString();
  }
}
