-- add a public and non-public user for testing.
INSERT INTO users (id, email, pass_crypt, creation_time, display_name, data_public)
VALUES (1, 'user_1@example.com', '', '2013-11-14T02:10:00Z', 'user_1', true),
       (2, 'user_2@example.com', '', '2013-11-14T02:10:00Z', 'user_2', false),
       (3, 'user_3@example.com', '', '2015-09-05T20:37:00Z', 'user_3', true);

-- give the users some changesets
INSERT INTO changesets (id, user_id, created_at, closed_at)
VALUES (1, 1, '2013-11-14T02:10:00Z', '2013-11-14T03:10:00Z'),
       (2, 1, '2013-11-14T02:10:00Z', '2013-11-14T03:10:00Z'),
       (3, 1, '2013-11-14T02:10:00Z', '2013-11-14T03:10:00Z'),
       (4, 2, '2013-11-14T02:10:00Z', '2013-11-14T03:10:00Z');

-- and some tags on those changesets
INSERT INTO changeset_tags (changeset_id, k, v)
VALUES (2, 'test_key', 'test_value'),
       (2, 'test_key2', 'test_value2');

-- and a discussion on one of those changesets
INSERT INTO changeset_comments (id, changeset_id, author_id, body, created_at, visible)
VALUES (1, 3, 3, 'a nice comment!', '2015-09-05T20:37:01Z', true),
       (2, 3, 3, 'a nasty comment', '2015-09-05T20:37:10Z', false);

-- add some nodes for each of the users
INSERT INTO current_nodes (id, latitude, longitude, changeset_id, visible, "timestamp", tile, version)
VALUES (1,       0,       0, 1, true,  '2013-11-14T02:10:00Z', 3221225472, 1),
       (2, 1000000, 1000000, 1, true,  '2013-11-14T02:10:01Z', 3221227032, 1),
       (3,       0,       0, 2, false, '2015-03-02T18:27:00Z', 3221225472, 2),
       (4,       0,       0, 4, true,  '2015-03-02T19:25:00Z', 3221225472, 1);

-- update the number of edits
UPDATE changesets
  SET num_changes = (
    SELECT count(*)
    FROM current_nodes
    WHERE changeset_id = changesets.id);
