INSERT INTO RFID.Tag (idTag, room_name, description, destination_node, username) VALUES
			('E2000019171302070480B7BF', 'B512', 'Acesta este B512', true, 'admin'),
            ('E2000019171302140480BF13', 'B509', 'Acesta este B509', true, 'admin'),
            ('E2000019171302000480AEA0', 'Auditorium', 'Acesta este Auditorium', false, 'admin'),
            ('E2000019171302010480AF7C', 'Secretariat', 'Acesta este Secretariatul', true, 'user');   
            
INSERT INTO RFID.User (username, password) VALUES
			('admin', 'd82494f05d6917ba02f7aaa29689ccb444bb73f20380876cb05d1f37537b7892'),
            ('user', 'e172c5654dbc12d78ce1850a4f7956ba6e5a3d2ac40f0925fc6d691ebb54f6bf');
