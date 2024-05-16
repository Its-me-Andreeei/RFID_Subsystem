package org.rfid_database_backend.rfid.requests;

import org.rfid_database_backend.rfid.dto.Tag;
import org.rfid_database_backend.rfid.dao.TagManager;
import org.springframework.stereotype.Component;

@Component
public class CheckTagRequest implements Request {
    final
    TagManager tagManager;

    public CheckTagRequest(TagManager tagManager) {
        this.tagManager = tagManager;
    }

    @Override
    public String processRequest(String data) {
        String result;
        Tag tag;
        tag = tagManager.getTagByID(data);
        if (tag == null) {
            result = "tag:\n";
        }
        else {
            result = "tag:" + tag.getRoomName() + ":" + tag.getDescription();
            if(tag.isDestinationNode()) {
                result += ":1"+ "\n";
            }
            else
            {
                result += ":0"+ "\n";
            }
        }
        System.out.println("Data to be sent: "+result);
        return result;
    }

}
