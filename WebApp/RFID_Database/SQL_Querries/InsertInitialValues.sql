#DELETE FROM RFID.Route;
#DELETE FROM RFID.Tag;

INSERT INTO RFID.Route (idRoute, route_name) 
VALUES 	(1, "Secretariat"),
		(2, "Auditorium"),
        (3, "B512");

INSERT INTO RFID.Tag (idTag, room_name, description, destination_node, idRoute)
VALUES	("E2000019171302070480B7BF", "Secretariat", "Acesta este secretariatul", TRUE, 1),
		("E2000019171302140480BF13", "Auditorium", "Acesta este Auditorium", TRUE, 2),
		("E2000019171302010480AF7C", "B512", "Acesta este B512", TRUE, 3),
        ("E2000019171302000480AEA0", "B509", "Acesta este B509", FALSE, 3);