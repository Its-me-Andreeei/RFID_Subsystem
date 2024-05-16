import {Component, OnInit, ViewChild} from '@angular/core';
import {Router} from "@angular/router";
import {Tag} from "../Models/Tag";
import {TagService} from "../Services/tag.service";
import {AuthService} from "../Services/auth.service";
import {MatPaginator} from "@angular/material/paginator";

@Component({
  selector: 'app-room-manager',
  templateUrl: './room-manager.component.html',
  styleUrl: './room-manager.component.css'
})
export class RoomManagerComponent implements OnInit {
  tags: Tag[] = [];
  columnsToDisplay = ['idTag', 'roomName', 'description', 'destinationNode'];
  clickedRows = new Set<Tag>();
  newTag: Tag = { idTag: '', roomName: '', description: '', destinationNode: false};
  currentUser : String | null | undefined;

  constructor(private authService: AuthService, private tagService: TagService, private router: Router) {
  }

  ngOnInit(): void {
    if (this.authService.getLoginStatus() === 'true')
    {
      this.currentUser = this.authService.getUsername();
      this.tagService.getAllTags().subscribe(
        response => {
          this.tags = response
        }
      );
    }
    else
    {
      this.router.navigate(['login']);
    }
  }

  onClickDelete() {
    // Iterate over each element in the clickedRows set
    this.clickedRows.forEach(tag => {
      // Call the deleteTag method for each tag and subscribe to the observable
      this.tagService.deleteTag(tag.idTag).subscribe(
        () => {
          // Delete operation completed successfully
          console.log(`Tag with ID ${tag.idTag} deleted successfully.`);
          // Remove the tag from the clickedRows set
          this.clickedRows.delete(tag);
          this.tagService.getAllTags().subscribe(
            response => {
              this.tags = response;
            }
          )
        },
      );
    });
  }

  onSubmit() {

      console.log(this.newTag);
      if(this.newTag.idTag !=="" || this.newTag.roomName !== "" || this.newTag.description !== "")
      {
        this.tagService.addNewTag(this.newTag).subscribe(
        response => {
          console.log(response);
          if (response === "CREATED") {
            console.log("Tag added successfully");
            this.tagService.getAllTags().subscribe(
              response => { this.tags = response;}
              )
            }
          }
        )
      }
    if (this.newTag.idTag != '' && this.newTag.roomName != '' && this.newTag.description != '') {
            this.newTag.idTag = '';
            this.newTag.roomName = '';
            this.newTag.description = '';
            this.newTag.destinationNode = false;
    }
  }

  logOff() {
    this.authService.logOff();
    this.router.navigate(['login']);
  }

  protected readonly onchange = onchange;
}
