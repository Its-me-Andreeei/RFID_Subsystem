package org.rfid.dao;

import com.mysql.cj.jdbc.StatementImpl;
import org.rfid.dto.Tag;

import java.sql.*;
import java.util.Properties;

public class TagManager {
    private ResultSet resultSet;
    private Connection connection;

    public TagManager() {
        /*Make the connection to Database*/
        Properties properties = new Properties();
        properties.put("user", "root");
        properties.put("password", "K@rolyi120i");
        try {
            final String connectionString = "jdbc:mysql://127.0.0.1:3306";
            connection = DriverManager.getConnection(connectionString, properties);
        } catch (SQLException e)
        {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }
    }


    public Tag getTag(String tagID) {
        Tag tag = new Tag();
        try {
            Statement statement = connection.createStatement();
            resultSet = statement.executeQuery("SELECT * FROM RFID.Tag WHERE idTag = '" + tagID + "'");
            if (resultSet.next()) {
                tag.setIdTag(resultSet.getString("idTag"));
                tag.setIdRoute(resultSet.getInt("idRoute"));
                tag.setDescription(resultSet.getString("description"));
                tag.setRoomName(resultSet.getString("room_name"));
                tag.setDestinationNode(resultSet.getBoolean("destination_node"));
            }
            else
            {
                tag = null;
                System.err.println("No tag found for ID in Database: " + tagID);
            }
        }
        catch (SQLException e) {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }

        return tag;
    }
}
