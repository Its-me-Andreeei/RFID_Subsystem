package org.rfid_database_backend.backend.services;

import lombok.Getter;
import org.jetbrains.annotations.NotNull;
import org.rfid_database_backend.rfid.dao.TagManager;
import org.rfid_database_backend.rfid.dto.User;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Getter
@Service
public class UserService {
    private final TagManager tagManager;
    private String currentLoggedInUserName;
   private boolean isConnected = false;

   @Autowired
    public UserService(TagManager tagManager) {
        this.tagManager = tagManager;
    }

    public boolean verifyCredentials(@NotNull User user) {
       boolean result = false;

       User dbuser = tagManager.getUserByName(user.getName());
       if((dbuser != null) && (user.getName().equals(dbuser.getName()) && user.getPassword().equals(dbuser.getPassword())))
       {
           currentLoggedInUserName = user.getName();
           isConnected = true;
           result = true;
       }

        return result;
    }
}
