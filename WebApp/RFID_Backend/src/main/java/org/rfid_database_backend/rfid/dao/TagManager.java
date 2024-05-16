package org.rfid_database_backend.rfid.dao;

import org.jetbrains.annotations.NotNull;
import org.rfid_database_backend.rfid.dto.Tag;
import org.rfid_database_backend.rfid.dto.User;
import org.springframework.stereotype.Repository;

import java.sql.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

@Repository
public class TagManager {
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


    public Tag getTagByID(@NotNull String tagID) {
        Tag tag = new Tag();
        ResultSet resultSet;
        try(Statement statement = connection.createStatement()) {

            resultSet = statement.executeQuery("SELECT * FROM RFID.Tag WHERE idTag = '" + tagID + "'");

            if (resultSet.next()) {
                tag.setIdTag(resultSet.getString("idTag"));
                tag.setDescription(resultSet.getString("description"));
                tag.setRoomName(resultSet.getString("room_name"));
                tag.setDestinationNode(resultSet.getBoolean("destination_node"));
            }
            else
            {
                tag = null;
            }
        }
        catch (SQLException e) {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }

        return tag;
    }

    public User getUserByName(@NotNull String userName) {
        User user = new User();
        ResultSet resultSet;
        try(Statement statement = connection.createStatement())
        {
            resultSet = statement.executeQuery("SELECT * FROM RFID.User WHERE username = '" + userName + "'");
            if(resultSet.next())
            {
                user.setName(resultSet.getString("username"));
                user.setPassword(resultSet.getString("password"));
            }
            else
            {
                user = null;
            }
        }
        catch (SQLException e) {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }
        return user;
    }

    public boolean addUser(@NotNull User user) {
        int numberOfAffectedRows = 0;
        boolean result = false;
        try(Statement statement = connection.createStatement()) {
            if(getUserByName(user.getName()) == null) {
                numberOfAffectedRows = statement.executeUpdate("INSERT INTO RFID.User (username, password) VALUE (\"%s\",\"%s\")".formatted(user.getName(), user.getPassword()));

                if (numberOfAffectedRows == 1) {
                    result = true;
                } else {
                    System.err.println("Too many tags were affected by insert operation");
                }
            }

        }catch (SQLException e)
        {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }
        return result;
    }

    public boolean insertTag(@NotNull Tag tag, @NotNull String username) {
        int numberOfAffectedRows = 0;
        boolean result = false;
        try(Statement statement = connection.createStatement()) {
            if(getUserByName(username) != null && getTagByID(tag.getIdTag()) == null) {
                numberOfAffectedRows = statement.executeUpdate("INSERT INTO RFID.Tag (idTag, room_name, description, destination_node, username) VALUE (\"%s\",\"%s\",\"%s\",%d, \"%s\")".formatted(tag.getIdTag(), tag.getRoomName(), tag.getDescription(), (tag.isDestinationNode() ? 1 : 0), username));

                if (numberOfAffectedRows == 1) {
                    result = true;
                } else {
                    System.err.println("Too many tags were affected by insert operation");
                }
            }

        }catch (SQLException e)
        {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }
    return result;
    }

    public boolean updateTag(@NotNull Tag tag) {
        int numberOfAffectedRows = 0;
        boolean result = false;
        Tag currentTag;

        try(Statement statement = connection.createStatement())
        {
            if(tag.getIdTag() != null)
            {
                currentTag = getTagByID(tag.getIdTag());
                if(currentTag != null) {
                    currentTag.setDescription(tag.getDescription());
                    currentTag.setRoomName(tag.getRoomName());
                    currentTag.setDestinationNode(tag.isDestinationNode());
                    numberOfAffectedRows = statement.executeUpdate("UPDATE RFID.Tag SET description=\"" + tag.getDescription() + "\", room_name=\"" + tag.getRoomName() + "\", destination_node=" + (tag.isDestinationNode()?1:0) + " WHERE idTag=\"" + tag.getIdTag() + "\"");
                    if(numberOfAffectedRows == 1) {
                        result = true;
                    }
                    else {
                        System.err.println("Too many tags were affected by insert operation");
                    }
                }
            }
        }
        catch (SQLException e) {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }

        return result;
    }

    public boolean deleteTagByIDandName(@NotNull String tagEpc, String userName) {
        int numberOfAffectedRows = 0;
        boolean result = false;

        try(Statement statement = connection.createStatement())
        {
            if(getTagByID(tagEpc) != null) {
                numberOfAffectedRows = statement.executeUpdate("DELETE FROM RFID.Tag WHERE idTag=\"" + tagEpc + "\" AND username='" + userName + "'");
                if(numberOfAffectedRows == 1) {
                    result = true;
                }
                else
                {
                    System.err.println("Too many tags were affected by insert operation");
                }
            }
        }
        catch (SQLException e)
        {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }
        return result;
    }

    public List<Tag> getAllTagsByUser(@NotNull String userName) {
        List<Tag> tags = new ArrayList<>();
        ResultSet resultSet;

        try(Statement statement = connection.createStatement())
        {
            resultSet = statement.executeQuery("SELECT * FROM RFID.Tag WHERE username=\"" + userName + "\"");
            while (resultSet.next()) {
                Tag tag = new Tag();
                tag.setIdTag(resultSet.getString("idTag"));
                tag.setRoomName(resultSet.getString("room_name"));
                tag.setDescription(resultSet.getString("description"));
                tag.setDestinationNode(resultSet.getBoolean("destination_node"));
                tags.add(tag);
            }
        }
        catch (SQLException e) {
            System.err.println("Could not establish connection to SQL database: " + e.getMessage());
        }
        return tags;
    }
}

