-- Wrong guesses against a user's outstanding sign-in codes. After five, OtpService burns every outstanding code
-- for the user and a new one must be requested.
ALTER TABLE auth_tokens ADD COLUMN attempts INT NOT NULL DEFAULT 0;
