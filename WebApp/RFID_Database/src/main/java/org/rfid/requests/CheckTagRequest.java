package org.rfid.requests;

import org.rfid.dao.TagManager;
import org.rfid.dto.Tag;

public class CheckTagRequest implements Request {
    private String data;
    TagManager tagManager;

    public CheckTagRequest() {
        tagManager = new TagManager();
    }

    @Override
    public String processRequest(String data) {
        String result;
        Tag tag;
        tag = tagManager.getTag(data);
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
