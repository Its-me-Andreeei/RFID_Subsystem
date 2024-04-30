package org.rfid.dto;

import lombok.Data;

@Data
public class Tag {
    private String idTag;
    private String roomName;
    private String description;
    private boolean destinationNode;
    private int idRoute;
}
