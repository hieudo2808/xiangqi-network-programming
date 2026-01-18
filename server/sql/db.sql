CREATE DATABASE XiangqiDB;
GO
 
USE XiangqiDB;
GO

CREATE TABLE Users (
    user_id INT IDENTITY(1,1) PRIMARY KEY,
    username NVARCHAR(64) UNIQUE NOT NULL, 
    email NVARCHAR(128) UNIQUE NOT NULL, 
    password_hash NVARCHAR(128) NOT NULL, 
    rating INT DEFAULT 1200,
    wins INT DEFAULT 0,
    losses INT DEFAULT 0,
    draws INT DEFAULT 0,
    created_at DATETIME DEFAULT GETDATE(),
    INDEX IX_Users_Rating (rating DESC),
    INDEX IX_Users_Username (username)
);
GO

CREATE TABLE Matches (
    match_id NVARCHAR(64) PRIMARY KEY,
    red_user_id INT NOT NULL,
    black_user_id INT NOT NULL,
    result NVARCHAR(16) CHECK (result IN ('red_win', 'black_win', 'draw', 'ongoing')), 
    moves_json NVARCHAR(MAX),
    started_at NVARCHAR(32),
    ended_at NVARCHAR(32),
    FOREIGN KEY (red_user_id) REFERENCES Users(user_id),
    FOREIGN KEY (black_user_id) REFERENCES Users(user_id),
    INDEX IX_Matches_Users (red_user_id, black_user_id),
    INDEX IX_Matches_StartedAt (started_at DESC)
);
GO

CREATE TABLE Sessions (
    session_token NVARCHAR(64) PRIMARY KEY,
    user_id INT NOT NULL,
    created_at DATETIME DEFAULT GETDATE(),
    expires_at DATETIME NOT NULL,
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    INDEX IX_Sessions_UserID (user_id),
    INDEX IX_Sessions_ExpiresAt (expires_at)
);
GO
 
CREATE TABLE active_matches (
        match_id VARCHAR(64) PRIMARY KEY,
        red_user_id INT NOT NULL,
        black_user_id INT NOT NULL,
        current_turn VARCHAR(6) DEFAULT 'red',
        red_time_ms INT DEFAULT 600000,
        black_time_ms INT DEFAULT 600000,
        move_count INT DEFAULT 0,
        moves_json NVARCHAR(MAX),
        rated BIT DEFAULT 1,
        started_at DATETIME DEFAULT GETDATE(),
        last_move_at DATETIME DEFAULT GETDATE()
    );
 
GO;
 
INSERT INTO Users (username, email, password_hash, rating, wins, losses, draws)
VALUES ('testuser', 'test@example.com', 'd91da15b07b01fb413e31be527f05b9563b0515652b0515672b0bcb1ca2a6185', 1200, 0, 0, 0);
-- Pass: test123 of testUser
GO;

CREATE PROCEDURE CleanupExpiredSessions
AS
BEGIN
    DELETE FROM Sessions WHERE expires_at < GETDATE();
END;
GO
 
PRINT 'Database schema created successfully!';