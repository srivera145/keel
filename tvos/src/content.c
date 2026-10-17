/* KEEL OS — content catalog. Every title, show, album and photo is original. */
#include "assets.h"
#include "icons.h"

#define SC(k, t, e, s, sx, sy) {k, t, e, s, sx, sy, 0, 0}

Movie g_movies[] = {
    {"Northern Light", "Some nights never end.", "Drama", "2026", "PG-13", "2h 4m",
     "A glaciologist stranded at an Arctic research station finds herself racing the polar night to decode a signal hidden in the aurora.",
     "Ines Calloway", {"Mara Lindqvist", "Theo Okafor", "June Park", "Ravi Sethi"}, SC(SC_AURORA, TOD_NIGHT, EL_FIGURE, 11, 1.1f, 0.30f), 0, 4.6f, 1},
    {"The Last Lighthouse", "Keep the light burning.", "Mystery", "2025", "PG-13", "1h 52m",
     "When the keeper of a remote lighthouse vanishes, his estranged daughter returns to the island and uncovers a century of buried promises.",
     "Owen Hartley", {"Sadie Marlowe", "Elias Brandt", "Noor Haddad", "Finn Gallagher"}, SC(SC_OCEAN, TOD_DUSK, EL_LIGHTHOUSE, 22, 0.55f, 0.48f), 0, 4.3f, 0},
    {"Solar Drift", "Beyond the last horizon.", "Sci-Fi", "2026", "PG-13", "2h 18m",
     "A salvage pilot crosses a desert world under twin moons to recover a ship that should not exist, and the crew that should not be alive.",
     "Kaito Mercer", {"Zara Velez", "Cole Whitaker", "Amara Diallo", "Lev Petrov"}, SC(SC_DESERT, TOD_GOLDEN, EL_PLANET, 33, 1.25f, 0.42f), 1, 4.7f, 1},
    {"Neon Hearts", "Love at 120 bpm.", "Romance", "2025", "PG", "1h 46m",
     "Two rival DJs are forced to share a single summer residency in a city that never sleeps, and a song neither can finish alone.",
     "Lucia Moretti", {"Kai Nakamura", "Bella Ortiz", "Dev Anand", "Rosa Klein"}, SC(SC_SYNTH, TOD_DUSK, EL_NONE, 44, 0.89f, 0.52f), 2, 4.1f, 0},
    {"Ember Ridge", "The mountain remembers.", "Adventure", "2026", "PG-13", "2h 9m",
     "A disgraced mountain guide leads one final expedition up the peak that took her brother, with a storm and a secret close behind.",
     "Hollis Grant", {"Nadia Rourke", "Sam Achebe", "Ivy Laurent", "Marco Silva"}, SC(SC_MOUNTAINS, TOD_GOLDEN, EL_FIGURE, 55, 1.3f, 0.48f), 1, 4.5f, 1},
    {"Deep Signal", "Something answered.", "Thriller", "2025", "R", "1h 58m",
     "Aboard a deep-space listening post, a lone engineer receives a message in her own voice, sent from three days in the future.",
     "Anika Strand", {"Hana Sato", "Wes Mbeki", "Petra Novak", "Luis Romero"}, SC(SC_SPACE, TOD_NIGHT, EL_SHIP, 66, 1.2f, 0.4f), 2, 4.4f, 0},
    {"Paper Kingdoms", "Every fold tells a story.", "Family", "2026", "G", "1h 34m",
     "A shy origami artist discovers her paper creatures come alive at night, and the kingdom they build needs her help before dawn.",
     "Pip Okonkwo", {"Lily Chen", "Max Fairbanks", "Gus Moreau", "Ada Quinn"}, SC(SC_PAPER, TOD_DAWN, EL_TREE, 77, 1.2f, 0.36f), 0, 4.8f, 1},
    {"Glass Harbor", "The city has a price.", "Crime", "2025", "R", "2h 11m",
     "A harbor detective with a perfect record bends one rule to save a witness, and the whole glittering skyline starts to crack.",
     "Dario Venn", {"Jonah Reyes", "Clara Voss", "Imani Brooks", "Tom Albrecht"}, SC(SC_CITY, TOD_NIGHT, EL_NONE, 88, 1.35f, 0.3f), 1, 4.2f, 0},
    {"Wildwood", "Not all who wander are lost.", "Fantasy", "2026", "PG", "1h 57m",
     "Lost in an ancient forest that rearranges itself each night, two siblings follow a trail of lanterns toward a home that may not want them back.",
     "Freya Holm", {"Oscar Bell", "Mina Farouk", "Ellis Ward", "Saoirse Doyle"}, SC(SC_FOREST, TOD_DAWN, EL_FIGURE, 99, 1.3f, 0.25f), 2, 4.6f, 0},
    {"Halcyon Days", "One summer. No regrets.", "Comedy", "2025", "PG-13", "1h 41m",
     "Four friends inherit a crumbling beach hotel for one summer and must turn a profit by Labor Day, or lose it to a smug developer.",
     "Rory Callahan", {"Tess Mulroney", "Big Andre", "Priya Kapoor", "Leo Strauss"}, SC(SC_OCEAN, TOD_DAY, EL_BOAT, 110, 1.0f, 0.3f), 0, 3.9f, 0},
    {"Copper Sky", "Justice rides at dusk.", "Western", "2026", "R", "2h 2m",
     "An aging bounty hunter and a runaway telegraph operator cross the canyon country carrying a confession that could hang a governor.",
     "Wyatt Keene", {"Hank Delgado", "Ruby Ames", "Silas Crow", "Ana Vega"}, SC(SC_CANYON, TOD_GOLDEN, EL_FIGURE, 121, 0.4f, 0.5f), 1, 4.3f, 1},
    {"Orbit of Us", "Distance is relative.", "Sci-Fi", "2025", "PG-13", "2h 6m",
     "Separated by a failed launch, an astronaut in orbit and his daughter on the ground learn to talk again through a borrowed radio telescope.",
     "Mei Lin", {"Ben Adeyemi", "Ella Novak", "Arjun Rao", "Sofia Bruni"}, SC(SC_SPACE, TOD_MAGIC, EL_PLANET, 132, 0.6f, 0.45f), 0, 4.5f, 0},
    {"Low Tide", "What the sea gives back.", "Drama", "2026", "PG-13", "1h 49m",
     "A fishing town's fragile peace is tested when a storm uncovers a wreck off the coast, and with it the truth about a 20-year-old rescue.",
     "Maeve Sullivan", {"Callum Reid", "Iris Novak", "Paulo Neves", "Grace Ahn"}, SC(SC_OCEAN, TOD_DAWN, EL_BOAT, 143, 1.4f, 0.5f), 1, 4.0f, 0},
    {"Pale Summit", "Higher. Colder. Alone.", "Thriller", "2025", "R", "1h 55m",
     "Trapped above the clouds after an avalanche, a climbing team realizes one of them caused it on purpose.",
     "Gunnar Ess", {"Liv Halvorsen", "Marcus Penn", "Yuki Mori", "Dana Fox"}, SC(SC_MOUNTAINS, TOD_DAY, EL_FIGURE, 154, 0.4f, 0.25f), 2, 4.2f, 0},
    {"Afterglow", "Stay until the light fades.", "Romance", "2026", "PG-13", "1h 52m",
     "Two strangers miss the last bus out of a desert town and spend one luminous night discovering they've met before.",
     "Selma Aziz", {"Rafael Cruz", "Nora Klein", "Omar Said", "Tilda Moss"}, SC(SC_DESERT, TOD_DUSK, EL_FIGURE, 165, 0.7f, 0.52f), 1, 4.4f, 1},
    {"Velvet Static", "Turn it up.", "Music", "2025", "PG-13", "1h 38m",
     "A behind-the-scenes portrait of an underground synth collective on the eve of the one show that could make or break them.",
     "Juno Park", {"The Static", "Mila Rae", "DJ Halo", "Kit Navarro"}, SC(SC_SYNTH, TOD_MAGIC, EL_NONE, 176, 0.89f, 0.5f), 0, 4.1f, 0},
    {"Kingfisher", "Small wings. Big world.", "Family", "2026", "G", "1h 29m",
     "A young kingfisher who can't yet fly sets out on foot across a vast forest lake to find the river where he hatched.",
     "Bea Lindgren", {"Tommy Hale", "Rosa Diaz", "Grandpa Joe", "Eve Sun"}, SC(SC_LAKE, TOD_DAY, EL_TREE, 187, 1.2f, 0.28f), 2, 4.7f, 0},
    {"Starfall", "The sky is falling.", "Sci-Fi", "2026", "PG-13", "2h 14m",
     "When a meteor shower knocks out every satellite on Earth, a retired astronomer and her grandson chart the way home by the stars alone.",
     "Victor Hale", {"Edith Crane", "Milo Grant", "Asha Bell", "Kenji Ito"}, SC(SC_AURORA, TOD_MAGIC, EL_FIGURE, 198, 0.5f, 0.3f), 1, 4.6f, 1},
    {"Night Train to Aurelia", "Next stop: the truth.", "Mystery", "2025", "PG-13", "2h 0m",
     "A sleeper train crossing the mountains at midnight, eleven passengers, and a conductor who insists there were only ten.",
     "Agnes Pell", {"Henri Dubois", "Lena Fischer", "Oyelaran Ade", "Mo Chen"}, SC(SC_MOUNTAINS, TOD_NIGHT, EL_TOWER, 209, 1.4f, 0.22f), 0, 4.3f, 0},
    {"The Quiet Hour", "Before the city wakes.", "Drama", "2026", "PG", "1h 47m",
     "In the hour before dawn, a night-shift nurse, a baker and a jazz pianist cross paths, and each of their lives turns on a single kindness.",
     "Camille Roy", {"Ada Osei", "Luca Bianchi", "Martha Kim", "Joel Rivers"}, SC(SC_CITY, TOD_DAWN, EL_NONE, 220, 0.5f, 0.5f), 2, 4.2f, 0},
    {"Saltwater Saints", "Ride the swell.", "Adventure", "2025", "PG-13", "1h 56m",
     "A crew of misfit surfers chases the legendary wave said to appear only once a decade, off a coast no map will show.",
     "Kahu Tane", {"Keala Moana", "Rex Hart", "Lani Cruz", "Dom Price"}, SC(SC_OCEAN, TOD_GOLDEN, EL_NONE, 231, 0.9f, 0.5f), 0, 4.0f, 0},
    {"Wanderlight", "Follow the glow.", "Fantasy", "2026", "PG", "1h 43m",
     "A lamplighter's apprentice discovers that every lantern in her city holds a captured star, and someone is setting them free.",
     "Odile March", {"Fern Ashby", "Tobias Reed", "Mira Sol", "Quentin Lake"}, SC(SC_PAPER, TOD_DUSK, EL_TOWER, 242, 0.5f, 0.32f), 1, 4.5f, 1},
    {"Iron Meridian", "Hold the line.", "Action", "2026", "R", "2h 7m",
     "A demolition expert must cross a collapsing border city in a single night to bring her team home before the bridges fall.",
     "Rex Calder", {"Vera Stone", "Ike Mensah", "Bo Lindahl", "Carmen Ruiz"}, SC(SC_CITY, TOD_DUSK, EL_TOWER, 253, 1.1f, 0.45f), 2, 4.1f, 0},
    {"Tidal", "The planet breathes.", "Documentary", "2025", "G", "1h 32m",
     "A breathtaking journey along the edges of the world's oceans, where the tide decides everything that lives and everything that doesn't.",
     "Naomi Reef", {"Narrated by", "Sir Alan Cove", "", ""}, SC(SC_OCEAN, TOD_MAGIC, EL_NONE, 264, 1.2f, 0.4f), 1, 4.8f, 0},
};
const int g_nmovies = COUNT(g_movies);

const int g_featured[] = {4, 2, 0, 6, 10};
const int g_nfeatured = COUNT(g_featured);

const ContinueItem g_continue[] = {
    {1, 0.62f, "42 min left"},
    {7, 0.35f, "1h 25m left"},
    {11, 0.82f, "22 min left"},
    {13, 0.18f, "1h 34m left"},
    {16, 0.5f, "45 min left"},
    {20, 0.71f, "33 min left"},
};
const int g_ncontinue = COUNT(g_continue);

enum { CH_NEWS, CH_NATURE, CH_SPORTS, CH_SPACE, CH_WEATHER, CH_OCEAN, CH_RETRO, CH_KIDS, CH_CINEMA };

Channel g_channels[] = {
    {"KEEL News 24", "NEWS", 101, CH_NEWS, HEX(0xE63946),
     {"World Tonight", "Market Watch", "The Briefing", "Global Desk", "Late Edition", "Tech Today", "Morning Wire", "Headlines"}},
    {"Planet Wild", "WILD", 102, CH_NATURE, HEX(0x2A9D8F),
     {"Mountain Kings", "The Great Migration", "Ridge Line", "Hidden Valleys", "Wild Seasons", "Eagle Country", "High Places", "Untamed"}},
    {"Arena Sports", "ARENA", 103, CH_SPORTS, HEX(0x2B7DE9),
     {"Live: City vs Harbor", "Match Day", "Goal Rush", "Post-Match Live", "The Dugout", "Highlights", "Kickoff", "Full Time"}},
    {"Cosmos HD", "COSMOS", 104, CH_SPACE, HEX(0x7B2FF7),
     {"Edge of the Universe", "Starlight", "Planet Hunters", "Dark Matter", "The Rings", "Deep Field", "Orbit", "Launch Pad"}},
    {"Weather Now", "WX", 105, CH_WEATHER, HEX(0xF4A261),
     {"Forecast Live", "Storm Tracker", "Weekend Outlook", "Weather Hour", "Sky Watch", "Morning Forecast", "Regional", "Travel Weather"}},
    {"Deep Blue", "BLUE", 106, CH_OCEAN, HEX(0x0096C7),
     {"Reef Life", "The Abyss", "Ocean Giants", "Kelp Forest", "Coral Cities", "Twilight Zone", "Currents", "Blue Planet Live"}},
    {"Retro Wave", "RETRO", 107, CH_RETRO, HEX(0xFF4FB4),
     {"Midnight Drive", "Synth Hour", "Neon Nights", "Outrun Radio", "Laser Disco", "Arcade Anthems", "Chrome Dreams", "Sunset Mix"}},
    {"Kids Club", "KIDS", 108, CH_KIDS, HEX(0xFFB703),
     {"Bouncy Town", "Shape Squad", "Rainbow Road", "Doodle Friends", "Sing Along", "Tiny Explorers", "Sleepy Stars", "Puzzle Pals"}},
    {"Cinema One", "CINE", 109, CH_CINEMA, HEX(0xC9A227),
     {"Ember Ridge", "Glass Harbor", "Solar Drift", "Wildwood", "Copper Sky", "Starfall", "Low Tide", "Afterglow"}},
};
const int g_nchannels = COUNT(g_channels);

Album g_albums[] = {
    {"Neon Drive", "Midnight Arcade", "Synthwave", 0, 104, 45, 1, HEX(0xFF4FB4), HEX(0x6A00F4), HEX(0xFFC14D),
     {"Chrome Horizon", "Night Shift", "Overdrive", "Afterhours", "Last Exit"}},
    {"Rainy Window", "Soft Static", "Lo-Fi", 1, 78, 50, 0, HEX(0x8EC5FC), HEX(0xE0C3FC), HEX(0x3D5A80),
     {"Cold Coffee", "Paper Planes", "Sunday Slow", "Drizzle", "Window Seat"}},
    {"Glacier", "Aurelia", "Ambient", 2, 62, 48, 1, HEX(0x48CAE4), HEX(0x90E0EF), HEX(0x023E8A),
     {"Ice Shelf", "Polar Bloom", "Whiteout", "Meltwater", "Long Night"}},
    {"Night Market", "Lanterns", "House", 3, 122, 41, 1, HEX(0xF72585), HEX(0x4CC9F0), HEX(0x3A0CA3),
     {"Street Food", "Lantern Glow", "Late Bus", "Neon Noodles", "Closing Time"}},
    {"Paper Moon", "Clara Weiss", "Piano", 4, 72, 53, 0, HEX(0xFFE5B4), HEX(0xF4A261), HEX(0x264653),
     {"Moonrise", "Letters", "Quiet Street", "Lullaby No. 2", "Morning Light"}},
    {"Pixel Hearts", "8-Bit Sun", "Chiptune", 5, 132, 48, 0, HEX(0x06D6A0), HEX(0xFFD166), HEX(0xEF476F),
     {"Level One", "Boss Fight", "Bonus Stage", "High Score", "Continue?"}},
    {"Aurora Suite", "Keel Orchestra", "Cinematic", 6, 84, 43, 1, HEX(0x80FFDB), HEX(0x5390D9), HEX(0x7400B8),
     {"Prelude", "The Ascent", "Northern Sky", "Requiem", "First Light"}},
    {"Sunset Boulevard", "Golden Hour", "Funk", 7, 112, 46, 0, HEX(0xFFBE0B), HEX(0xFB5607), HEX(0x8338EC),
     {"Cruise Control", "Palm Shade", "Velvet Groove", "Top Down", "Coastline"}},
};
const int g_nalbums = COUNT(g_albums);

Photo g_photos[] = {
    {"Mirror Lake", "Aurelia National Park", "Jun 14", SC(SC_LAKE, TOD_DAWN, EL_NONE, 501, 1.1f, 0.3f)},
    {"Dune Sea", "Erg Chebbi", "May 2", SC(SC_DESERT, TOD_GOLDEN, EL_NONE, 502, 1.2f, 0.45f)},
    {"Polar Night", "Tromsø", "Jan 9", SC(SC_AURORA, TOD_NIGHT, EL_NONE, 503, 1.0f, 0.3f)},
    {"Harbor Lights", "Lisbon", "Sep 21", SC(SC_CITY, TOD_DUSK, EL_NONE, 504, 0.8f, 0.45f)},
    {"Morning Fog", "Black Forest", "Oct 3", SC(SC_FOREST, TOD_DAWN, EL_NONE, 505, 1.3f, 0.25f)},
    {"Golden Coast", "Big Sur", "Aug 18", SC(SC_OCEAN, TOD_GOLDEN, EL_NONE, 506, 1.1f, 0.45f)},
    {"High Country", "Dolomites", "Jul 7", SC(SC_MOUNTAINS, TOD_DAY, EL_NONE, 507, 0.4f, 0.22f)},
    {"Red Rock", "Monument Valley", "Apr 11", SC(SC_CANYON, TOD_DUSK, EL_NONE, 508, 1.2f, 0.5f)},
    {"Alpenglow", "Patagonia", "Feb 27", SC(SC_LAKE, TOD_DUSK, EL_NONE, 509, 0.5f, 0.45f)},
    {"Blue Hour", "Hallstatt", "Dec 19", SC(SC_LAKE, TOD_NIGHT, EL_NONE, 510, 1.3f, 0.3f)},
    {"Sunrise Ridge", "Smoky Mountains", "Nov 5", SC(SC_MOUNTAINS, TOD_GOLDEN, EL_NONE, 511, 1.0f, 0.4f)},
    {"Calm Water", "Lofoten", "Mar 30", SC(SC_OCEAN, TOD_DAY, EL_NONE, 512, 0.6f, 0.45f)},
    {"Milky Way", "Atacama", "Jun 1", SC(SC_SPACE, TOD_NIGHT, EL_NONE, 513, 1.0f, 0.3f)},
    {"Emerald Valley", "Faroe Islands", "Jul 22", SC(SC_FOREST, TOD_DAY, EL_NONE, 514, 0.4f, 0.2f)},
    {"Last Light", "Sahara", "Oct 28", SC(SC_DESERT, TOD_DUSK, EL_NONE, 515, 0.5f, 0.5f)},
    {"Snowline", "Banff", "Jan 30", SC(SC_AURORA, TOD_MAGIC, EL_NONE, 516, 1.2f, 0.25f)},
};
const int g_nphotos = COUNT(g_photos);

AppInfo g_apps[] = {
    {"Movies", "Films & series", IC_FILM, HEX(0xFF6B6B), HEX(0xC0392B), 1},
    {"Live TV", "9 channels", IC_TV, HEX(0x4FACFE), HEX(0x2B5CE6), 2},
    {"Music", "Listen now", IC_MUSIC, HEX(0xFF5F9E), HEX(0xA531DC), 3},
    {"Photos", "Your memories", IC_PHOTO, HEX(0xFFD86F), HEX(0xFC6262), 4},
    {"Arcade", "Play together", IC_GAMEPAD, HEX(0x43E97B), HEX(0x16A085), -1},
    {"Weather", "18° · Sunny", IC_WEATHER, HEX(0x6DD5FA), HEX(0x2980B9), -1},
    {"Fitness", "Move every day", IC_FITNESS, HEX(0xFDC830), HEX(0xF37335), -1},
    {"Podcasts", "New episodes", IC_MIC, HEX(0xB06AB3), HEX(0x4568DC), -1},
    {"Kids", "Safe & fun", IC_KIDS, HEX(0xFFAFBD), HEX(0xFF6F91), -1},
    {"News", "Top stories", IC_NEWS, HEX(0x8E9EAB), HEX(0x3A4750), -1},
    {"Search", "Find anything", IC_SEARCH, HEX(0x89F7FE), HEX(0x66A6FF), 5},
    {"Settings", "Make it yours", IC_GEAR, HEX(0xBDC3C7), HEX(0x5D6D7E), 6},
};
const int g_napps = COUNT(g_apps);

Profile g_profiles[] = {
    {"Sofia", 0, HEX(0xFF9A8B), HEX(0xFF6A88)},
    {"Marcus", 1, HEX(0x74EBD5), HEX(0x3B8BEB)},
    {"Kids", 2, HEX(0xFDD819), HEX(0xE80505)},
    {"Guest", 3, HEX(0xC471F5), HEX(0x6A5AE0)},
};
const int g_nprofiles = COUNT(g_profiles);
