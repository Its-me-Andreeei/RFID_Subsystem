package org.rfid_database_backend.backend.controllers;

import org.jetbrains.annotations.NotNull;
import org.rfid_database_backend.backend.services.UserService;
import org.rfid_database_backend.rfid.dao.TagManager;
import org.rfid_database_backend.rfid.dto.User;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@CrossOrigin(origins = "http://localhost:4200")
@RestController
public class UserAuthController {
    private final TagManager tagManager;
    UserService userService;

    @Autowired
    public UserAuthController(UserService userService, TagManager tagManager) {
        this.userService = userService;
        this.tagManager = tagManager;
    }

    @PostMapping(value = "/login", consumes = "application/json")
    public ResponseEntity<HttpStatus> login(@NotNull @RequestBody User user)
    {
        if((user.getName() !=null) && (user.getPassword() != null)) {
            if (userService.verifyCredentials(user))
                return ResponseEntity.ok(HttpStatus.OK);
            else
                return ResponseEntity.ok(HttpStatus.UNAUTHORIZED);
        }
        else
            return ResponseEntity.ok(HttpStatus.UNAUTHORIZED);
    }

    @PostMapping(value = "/register", consumes = "application/JSON")
    public ResponseEntity<HttpStatus> register(@NotNull @RequestBody User user)
    {
        if((user.getName() !=null) && (user.getPassword() != null) && tagManager.addUser(user)) {
            return ResponseEntity.ok(HttpStatus.CREATED);
        }
        else {
            return ResponseEntity.ok(HttpStatus.CONFLICT);
        }
    }
}
