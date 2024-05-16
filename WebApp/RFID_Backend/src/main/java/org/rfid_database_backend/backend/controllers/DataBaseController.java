package org.rfid_database_backend.backend.controllers;

import org.jetbrains.annotations.NotNull;
import org.rfid_database_backend.backend.services.UserService;
import org.rfid_database_backend.rfid.dao.TagManager;
import org.rfid_database_backend.rfid.dto.Tag;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@CrossOrigin(origins = "http://localhost:4200")
@RestController
public class DataBaseController {
    private final TagManager tagManager;
    private final UserService userService;

    @Autowired
    public DataBaseController(TagManager tagManager, UserService userService) {
        this.tagManager = tagManager;
        this.userService = userService;
    }

    @GetMapping(value = "/tag/get/all")
    public ResponseEntity<List<Tag>> getAllTags() {
        return ResponseEntity.ok().body(tagManager.getAllTagsByUser(userService.getCurrentLoggedInUserName()));
    }

    @GetMapping(value = "/tag/get/{id}")
    public ResponseEntity<Tag> getTagById(@NotNull @PathVariable("id") String id) {
        return ResponseEntity.ok().body(tagManager.getTagByID(id));
    }

    @PostMapping(value = "/tag/add", consumes = "application/json")
    public ResponseEntity<HttpStatus> addTag(@NotNull @RequestBody Tag tag) {
        if(userService.isConnected())
        {
            if(tagManager.insertTag(tag, userService.getCurrentLoggedInUserName()))
            {
                return ResponseEntity.ok(HttpStatus.CREATED);
            }
            else
            {
                return ResponseEntity.ok(HttpStatus.CONFLICT);
            }
        }
        else
        {
            return ResponseEntity.ok(HttpStatus.UNAUTHORIZED);
        }
    }

    @PutMapping(value = "/tag/update", consumes = "application/json")
    public ResponseEntity<HttpStatus> updateTag(@NotNull @RequestBody Tag tag) {
        if(userService.isConnected())
        {
            if(tagManager.updateTag(tag))
            {
                return ResponseEntity.ok(HttpStatus.CREATED);
            }
            else
            {
                return ResponseEntity.ok(HttpStatus.CONFLICT);
            }
        }
        else
        {
            return ResponseEntity.ok(HttpStatus.UNAUTHORIZED);
        }
    }
    @DeleteMapping("/tag/delete/{id}")
    public ResponseEntity<HttpStatus> deleteTagById(@NotNull @PathVariable("id") String id) {
        if(userService.isConnected())
        {
            if(tagManager.getTagByID(id) != null)
            {
                if(tagManager.deleteTagByIDandName(id, userService.getCurrentLoggedInUserName()))
                {
                    return ResponseEntity.ok(HttpStatus.OK);
                }
                else
                {
                    return ResponseEntity.ok(HttpStatus.CONFLICT);
                }
            }
            else
            {
                return ResponseEntity.ok(HttpStatus.NO_CONTENT);
            }

        }
        else
        {
            return ResponseEntity.ok(HttpStatus.UNAUTHORIZED);
        }
    }
}
