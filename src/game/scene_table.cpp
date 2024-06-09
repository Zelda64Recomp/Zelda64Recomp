#include <cstdint>
#include <vector>
#include <string>
#include "zelda_debug.h"

std::vector<zelda64::AreaWarps> zelda64::game_warps {
    { "Clock Town", {
        {
            0, "Mayor's Residence", {
                "Entrance",
                "Post Couple's Mask",
            }
        },
        {
            8, "Honey & Darling's Shop", {
                "Entrance",
            }
        },
        {
            14, "Curiosity Shop", {
                "Entrance",
                "Back Entrance",
                "Peephole",
                "Exiting peephole",
            }
        },
        {
            36, "Milk Bar", {
                "Entrance",
            }
        },
        {
            40, "Treasure Chest Shop", {
                "Entrance",
                "After game",
            }
        },
        {
            44, "Clock Tower Roof", {
                "Entrance",
                "After cutscene"
            }
        },
        {
            54, "Deku Scrub Playground", {
                "Entrance",
                "Game Finished",
            }
        },
        {
            58, "Shooting Gallery", {
                "Entrance (with label)",
                "Entrance",
            }
        },
        {
            86, "Post Office", {
                "Entrance",
            }
        },
        {
            98, "Trading Post", {
                "Entrance (with label)",
                "Entrance",
            }
        },
        {
            108, "Lottery Shop", {
                "Entrance",
            }
        },
        {
            162, "Swordsman's School", {
                "Entrance",
            }
        },
        {
            188, "Stock Pot Inn", {
                "Front entrance",
                "From balcony",
                "Near granny",
                "Kitchen",
                "Anju and Anju's mother cutscene",
                "After Anju cutscene",
            }
        },
        {
            192, "Clock Tower", {
                "From lost woods (cutscene)",
                "Clock Town entrance",
                "After learning song of healing",
                "After moon falls",
                "First song of time cutscene",
                "From lost woods",
                "Happy Mask Salesman cutscene",
            }
        },
        {
            202, "Bomb Shop", {
                "Entrance (with label)",
                "Entrance",
            }
        },
        {
            210, "East Clock Town", {
                "From Termina Field",
                "From South Clock Town (South entrance)",
                "From Bombers' Hideout",
                "From South Clock Town (North entrance)",
                "From Treasure Chest Shop",
                "From North Clock Town",
                "From Honey & Darling's Shop",
                "From the Mayor's Residence",
                "From Town Shooting Gallery",
                "From Stock Pot Inn",
                "Stock Pot Inn balcony",
                "From Milk Bar",
            }
        },
        {
            212, "West Clock Town", {
                "From Termina Field",
                "From South Clock Town (South entrance)",
                "From South Clock Town (North entrance)",
                "From Swordsman's School",
                "From Curiosity Shop",
                "From Trading Post",
                "From Bomb Shop",
                "From Post Office",
                "From Lottery Shop",
                "From Termina Field (?)",
            }
        },
        {
            214, "North Clock Town", {
                "From Termina Field",
                "From East Clock Town",
                "From South Clock Town",
                "From Fairy's Fountain",
                "From Deku Scrub Playground",
                "Near Termina Field entrance",
                "Near Jim",
                "After Sakon cutscene",
            }
        },
        {
            216, "South Clock Town", {
                "From Clock Tower",
                "From Termina Field",
                "From East Clock Town (North entrance)",
                "From West Clock Town (North entrance)",
                "From North Clock Town",
                "From West Clock Town (South entrance)",
                "From Laundry Pool",
                "From East Clock Town (South entrance)",
                "Clock Tower balcony",
                "Owl Statue",
                "First song of time cutscene"
            }
        },
        {
            218, "Laundry Pool", {
                "From South Clock Town",
                "Curiosity Shop back entrance"
            }
        },
    }},
    { "Swamp", {
        {
            12, "Southern Swamp (After Woodfall Temple)", {
                "From Road",
                "In Front of Boat House",
                "Froom Woodfall",
                "From Lower Deku Palace",
                "From Upper Deku Palace",
                "From Magic Hags' Potion Shop",
                "Boat Ride",
                "From Woods of Mistery",
                "From Swamp Spider House",
                "From Ikanya Canyon",
                "Owl Statue",
            }
        },
        {
            4, "Hags' Potion Shop", {
                "Entrance",
            }    
        },
        {
            48, "Woodfall Temple", {
                "Entrance",
                "Deku Princess Room (First Visit)",
                "Deku Princess Room",
            }
        },
        {
            56, "Odolwa Arena", {
                "Entrance",
            }
        },
        {
            66, "Swamp Shooting Gallery", {
                "Entrance",
            }
        },
        {
            72, "Swamp Spider House", {
                "Entrance",
            }
        },
        {
            80, "Deku Palace", {
                "From Southern Swamp",
                "After getting caught",
                "From Deku King Chamber",
                "From Upper Deku King Chamber",
                "From Deku Shrine",
                "From Southern Swamp (Upper tunnel)",
                "From Left Grotto (Japanese)",
                "From Left Grotto Second Room (Japanese)",
                "From Right Grotto Second Room (Japanese)",
                "From Bean Seller Grotto",
                "From Right Grotto First Room (Japanese)",
            }
        },
        {
            118, "Deku Palace Royal Chamber", {
                "From Deku Palace",
                "From Upper Deku Palace",
                "After Releasing Monkey",
                "In Front of the King",
            }
        },
        {
            122, "Road to Southern Swamp", {
                "From Termina Field",
                "From Southern Swamp",
                "From Swamp Shooting Gallery",
            }
        },
        {
            132, "Southern Swamp (Before Woodfall Temple)", {
                "From Road to Southern Swamp",
                "In Front of Boat House",
                "From Woodfall",
                "From Deku Palace",
                "From Deku Palace (Shortcut)",
                "From Hags' Potion Shop",
                "Boat Ride",
                "From Woods of Mistery",
                "From Swamp Spider House",
                "From Ikana Canyon",
                "Owl Statue",
            }
        },
        {
            134, "Woodfall", {
                "From Southern Swamp",
                "In Mid-Air",
                "-From Fairy Mountain",
                "In Mid-Air (alternate)",
                "Owl Statue",
            }
        },
        {
            158, "Deku Shrine", {
                "From Deku Palace",
                "From Deku Palace"
            }
        },
        {
            168, "Swamp Tourist Center", {
                "Entrance",
                "Talking to Koume",
                "Talking to Tingle's Dad",
            }
        },
        {
            194, "Woods of Mystery", {
                "Entrance",
            }
        }
    }},
    { "Snowhead", {
        {
            50, "Road to Mountain Village", {
                "From Termina Field",
                "From Mountain Village",
            }
        },
        {
            60, "Snowhead Temple", {
                "Entrance (First Visit)",
                "Entrance",
            }
        },
        {
            82, "Mountain Smithy", {
                "Entrance",
            }
        },
        {
            94, "Goron Shrine", {
                "Main Entrance (First Visit)",
                "From Shop",
                "After Goron's Lullaby cutscene",
                "Main Entrance",
            }
        },
        {
            116, "Goron Shop", {
                "Entrance",
            }
        },
        {
            130, "Goht Arena", {
                "Entrance",
            }
        },
        {
            138, "Goron Village (After Snowhead Temple)", {
                "From Path to Goron Village",
                "In Mid-Air",
                "From Goron Shrine",
                "Over the Void",
                "In Front of Invisible Platforms",
            }
        },
        {
            148, "Goron Village (Before Snowhead Temple)", {
                "From Path to Goron Village",
                "In Front of Deku Flower",
                "From Goron Shrine",
                "From Lens of Truth",
                "In Front of Invisible Platforms",
            }
        },
        {
            150, "Goron Graveyard", {
                "-From Mountain Village",
                "-After Receiving Goron Mask",
            }
        },
        {
            154, "Mountain Village (Before Snowhead Temple)", {
                "In Front of Mountain Smithy",
                "Mountain Smithy",
                "From Path to Goron Village",
                "From Goron Graveyard",
                "From Path to Snowhead",
                "On the lake",
                "From Path to Mountain Village",
                "On the Lake (alternate)",
                "Owl Statue",
            }
        },
        {
            174, "Mountain Village (After Snowhead Temple)", {
                "Next to Lake",
                "From Mountain Smithy",
                "From Path to Goron Village",
                "From Goron Graveyard",
                "From Path to Snowhead",
                "Behind Waterfall",
                "From Path to Mountain Village",
                "Next to Lake (After Snowhead Cutscene)",
                "Owl Statue",
            }
        },
        {
            178, "Snowhead", {
                "From Path to Snowhead",
                "From Snowhead Temple",
                "From Fairy Fountain",
                "Owl Statue",
            }
        },
        {
            180, "Road to Goron Village (Before Snowhead Temple)", {
                "From Mountain Village",
                "From Goron Village",
                "From Goron Racetrack",
            }
        },
        {
            182, "Road to Goron Village (After Snowhead Temple)", {
                "From Mountain Village",
                "From Goron Village",
                "From Goron Racetrack",
            }
        },
        {
            208, "Goron Racetrack", {
                "From Path to Mountain Village",
                "Race Start",
                "Race End",
            }
        }
    }},
    { "Great Bay", {
        {
            34, "Pirates' Fortress (Outdoors)", {
                "From Exterior Pirate Fortress",
                "From Lower Hookshoot Room",
                "From Upper Hookshoot Room",
                "From Silver Rupee Room",
                "From Silver Rupee Room (alternate)",
                "From Room with Barrels",
                "From Room with Barrels (alternate)",
                "From Room with Barrels and Bridge",
                "From Room with Barrels and Bridge (alternate)",
                "Out of bounds",
                "Telescope",
                "Out of bounds (alternate)",
                "From Balcony in Exterior Pirate Fortress",
                "From Upper Hookshoot Room",
            }
        },
        {
            64, "Pirates' Fortress (Indoors)", {
                "Hookshoot Room",
                "Upper Hookshoot Room",
                "Silver Rupee Room",
                "100 Rupee Room (Next to Egg)",
                "Barrel Room",
                "Barrel Room (Next to Egg)",
                "Room with Barrels and Bridge",
                "Room with Barrels and Bridge (next to egg)",
                "Telescope",
                "Hidden Entrance ",
                "Telescope",
                "Unloaded Room",
            }
        },
        {
            68, "Pinnacle Rock", {
                "Entrance",
                "Respawn",
            }
        },
        {
            74, "Oceanside Spider House", {
                "Entrance",
            }
        },
        {
            88, "Marine Research Lab", {
                "Entrance",
            }
        },
        {
            96, "Zora Hall", {
                "From Zora Cape",
                "From Zora Cape with Turtle",
                "From Zora Shop",
                "From Lulu's Room",
                "From Evan's Room",
                "From Japa's Room",
                "From Mikau's & Tijo's Room",
                "Stage",
                "Stage (After Rehearsal)",
            }
        },
        {
            104, "Great Bay Coast", {
                "From Termina Field",
                "-zora cape",
                "From Zora Cape",
                "From Pinnacle Rock",
                "From Fisherman's Hut",
                "From Pirates' Fortress",
                "Next to Chuchu",
                "From Marine Lab",
                "From Oceanside Spider House",
                "Beach (Zora Mask Cutscene)",
                "Beach (After Zora Mask Cutscene)",
                "Owl Statue",
                "Thrown Out Pirates' Fortress",
                "Island (After jumping game)",
            }
        },
        {
            106, "Zora Cape", {
                "From Great Bay Coast",
                "From Zora Hall",
                "From Zora Hall with Turtle",
                "Next to Zora Game Site",
                "From Waterfall Rapids",
                "From Fairy Fountain",
                "From Owl Statue",
                "From Great Bay Temple",
                "After Beating Great Bay Temple",
                "After Beating Great Bay Temple",
            }
        },
        {
            112, "Pirates' Fortress (Entrance)", {
                "From Great Bay Coast",
                "From Pirates' Fortress",
                "From Secret Entrance",
                "From Underwater Jet",
                "Kicked out",
                "From Hookshot Platform in Pirates' Fortress",
                "From Telescope Room",
            }
        },
        {
            114, "Fisherman's Hut", {
                "Entrance",
            }
        },
        {
            140, "Great Bay Temple", {
                "Entrance (After intro)",
                "Entrance (With intro)"
            }
        },
        {
            142, "Waterfall Rapids", {
                "From Zora Cape",
                "Race Start",
                "Race End",
                "Race Won",
            }
        },
        {
            146, "Zora Hall (Room)", {
                "Mikau's Room",
                "Japas' Room",
                "Lulu's Room",
                "Evan's Room",
                "Japa's Room (after jam session)",
                "Zora's Shop",
                "Evan's Room (after composing song)",
            }
        },
        {
            184, "Gyorg Arena", {
                "Entrance",
                "Falling Cutscene",
            }
        },
        {
            190, "Great Bay (Pirate and Turtle Cutscene)", {
                "From Zora Cape",
            }
        }
    }},
    { "Ikana", {
        {
            32, "Ikana Canyon", {
                "From Ikana Road",
                "From Ghost Hut",
                "From Music Box House",
                "From Stone Tower",
                "Owl Statue",
                "From Beneath the Well",
                "From Sakon's Hideout",
                "After Beating Stone Tower Temple",
                "From Ikana Castle",
                "House Opening Cutscene",
                "Spring Water Cave (played Song of Storms)",
                "From Fairy Fountain",
                "From Secret Shrine",
                "From Spring Water Cave",
                "Spring Water Cave",
            }
        },
        {
            10, "Beneath the Graveyard", {
                "Day 2",
                "Day 1",
            }
        },
        {
            52, "Ancient Castle of Ikana", {
                "From Beneath the Well",
                "From Ikana Canyon",
                "Outdoors (From main entrance)",
                "Indoors (From main entrance)",
                "Indoors (From ceiling)",
                "Indoors (East Wing)",
                "Indoors (From throne room)"
            }
        },
        {
            38, "Stone Tower Temple (Normal)", {
                "Entrance (With Intro)",
                "Entrance (After Intro)",
            }
        },
        {
            42, "Stone Tower Temple (Inverted)", {
                "Entrance",
                "-boss room entrance"
            }
        },
        {
            90, "Beneath the Graveyard", {
                "Dampe's House",
                "Beneath the Graveyard",
            }
        },
        {
            102, "Twinmold Arena", {
                "Entrance",
                "Entrane (1?)"
            }
        },
        {
            128, "Ikana Graveyard", {
                "Road to Ikana",
                "From Grave 1 ",
                "From Grave 2",
                "From Grave 3",
                "From Dampe's House",
                "Keeta Defeated Cutscene",
            }
        },
        {
            144, "Beneath the Well", {
                "From Ikana Canyon",
                "From Ikana Castle",
            }
        },
        {
            152, "Sakon's Hideout", {
                "Entrance",
            }
        },
        {
            156, "Spirit House", {
                "Entrance",
                "Minigame start",
                "After minigame",
            }
        },
        {
            160, "Road to Ikana", {
                "From Termina Field",
                "From Ikana Canyon",
                "From Ikana Graveyard",
            }
        },
        {
            164, "Music Box House", {
                "Entrance",
            }
        },
        {
            166, "Ancient Castle of Ikana (Throne room)", {
                "Entrance",
            }
        },
        {
            170, "Stone Tower", {
                "From Ikana Canyon",
                "In Front of Temple",
                "From Stone Tower Temple",
                "Owl Statue",
            }
        },
        {
            172, "Stone Tower (Inverted)", {
                "In Front of Temple (Inverting Cutscene)",
                "From Stone Tower Temple",
            }
        },
        {
            186, "Secret Shrine behind Waterfall", {
                "Entrance",
            }
        }
    }},
    { "Overworld", {
        {
            76, "Astral Observatory", {
                "From Clock Town",
                "From Termina Field",
                "After telescope cutscene",
            }
        },
        {
            84, "Termina Field", {
                "From West Clock Town",
                "From Road to Southern Swapm",
                "From Great Bay Coast",
                "From Path to Mountain Village",
                "From Road to Ikana",
                "From Milk Road",
                "From South Clock Town",
                "From East Clock Town",
                "From North Clock Town",
                "From Observatory",
                "Use Telescope",
                "Near Ikana",
                "Moon Crash Cutscene (Game Over)",
                "Next to Ikana (After Cremia's Hug)",
                "Next to Road to Southern Swamp (After Skull Kid Cutscene)"
            }
        }
    }},
    { "Milk Road", {
        {
            6, "Romani Ranch (Indoors)", {
                "Barn",
                "House",
            }
        },
        {
            62, "Milk Road", {
                "From Termina Field",
                "From Romani Ranch",
                "From Gorman's Track (Track Exit)",
                "From Gorman's Track (Main Exit)",
                "At Owl Statue",
                "Behind Giant Rock",
                "Next to Owl Statue",
            }
        },
        {
            100, "Romani Ranch", {
                "Entrance",
                "After practice",
                "From Barn",
                "From House",
                "From Cucco Shack",
                "From Doggy Racetrack",
                "Near Barn (6?)",
                "Near Barn (7?)",
                "Near House (8?)",
                "Near Barn (9?)",
                "Talking to Romani",
                "Near Barn (11?)",
            }
        },
        {
            124, "Doggy Racetrack", {
                "From Romani Ranch",
                "Next to Track (After Race)",
            }
        },
        {
            126, "Cucco Shack", {
                "From Romani Ranch",
                "Talking to Grog (Getting Bunny Hood)",
            }
        },
        {
            206, "Gorman Track", {
                "From Milk Road",
                "Next to Gorman",
                "Next to Gorman (After Beating Race)",
                "From Milk Road (Behind Fence)",
                "From Milk Road (After Fence Cutscene)",
                "In the Middle of the Track"
            }
        }
    }},
    { "Moon", {
        {
            2, "Majora Arena", {
                "Entrance",
            }
        },
        {
            78, "Moon Trial (Deku)", {
                "Entrance",
            }
        },
        {
            120, "Moon Trial (Goron)", {
                "Entrance",
            }
        },
        {
            136, "Moon Trial (Zora)", {
                "Entrance",
                "Respawn",
            }
        },
        {
            198, "Moon Trial (Link)", {
                "Entrance",
            }
        },
        {
            200, "Moon", {
                "Entrance",
            }
        }
    }},
    { "Other", {
        {
            20, "Secret Grottos", {
                "Termina Field (Great Bay Gossip Stones)",
                "Termina Field (Swamp Gossip Stones)",
                "Termina Field (Ikana Gossip Stones)",
                "Termina Field (Mountain Gossip Stones)",
                "-generic grotto",
                "Road to Goron Village (Springwater)",
                "-maze straight (a)",
                "Termina Field near Dodongos",
                "-maze vines (lower)",
                "-business scrub",
                "-cows",
                "Termina Field under boulder (Piece of Heart)",
                "Deku Palace (Bean Seller)",
                "Termina Field (Peahat)",
                "-maze straight (b)",
                "-maze grotto (upper)",
                "-lens of truth",
            }
        },
        {
            46, "Intro Areas", {
                "Falling from Cliff Cutscene",
                "Before Entering Clock Tower",
                "After Being Transformed into Deku",
                "Before Entering Clock Tower (Void Respawn)",
                "South Clock Town (After First Song of Time)",
            }
        },
        {
            70, "Fairy's Fountain", {
                "Clock Town",
                "Woodfall",
                "Snowhead",
                "Great Bay Coast",
                "Ikana Canyon",
                "Clock Town (After cutscene)",
                "Woodfall (After cutscene)",
                "Snowhead (After cutscene)",
                "Great Bay Coast (After cutscene)",
                "Ikana Canyon (After cutscene)",
            }
        },
        {
            196, "Intro (Lost Woods)", {
                "Skull kid intro cutscene",
                "First Song of Time cutscene"
            }
        },
        {
            204, "Chamber of Giants", {
                "Entrance",
            }
        }
    }},
};

