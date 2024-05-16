import { Injectable } from '@angular/core';
import {HttpClient, HttpStatusCode} from "@angular/common/http";
import {Tag} from "../Models/Tag";
import {Observable} from "rxjs";

@Injectable({
  providedIn: 'root'
})
export class TagService {

  constructor(private http: HttpClient) { }

  getAllTags(): Observable<Tag[]>
  {
    return this.http.get<Tag[]>("http://localhost:8080/tag/get/all");
  }

  deleteTag(tag_id: string): Observable<any> {
    return this.http.delete<HttpStatusCode>("http://localhost:8080/tag/delete/" + tag_id);
  }

  addNewTag(tag: Tag): Observable<any> {
    return this.http.post<HttpStatusCode>("http://localhost:8080/tag/add", tag);
  }

}
