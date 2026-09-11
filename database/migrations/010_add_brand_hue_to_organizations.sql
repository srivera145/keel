-- Per-tenant brand hue, 0-360, emitted as Deck's --hue-brand on <html>. NULL keeps Keel's default hue.
ALTER TABLE organizations ADD COLUMN brand_hue SMALLINT UNSIGNED NULL DEFAULT NULL;
