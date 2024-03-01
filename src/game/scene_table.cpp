#include <cstdint>
#include <vector>
#include <string>
#include "recomp_debug.h"

std::vector<recomp::AreaWarps> recomp::game_warps {
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
                "From Song of Soaring",
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
                "-swamp road",
                "-boat house",
                "-woodfall",
                "-lower deku palace",
                "-upper deku palace",
                "-hags potion shop",
                "-boat cruise",
                "-woods of mystery",
                "-swamp spider house",
                "-ikana canyon",
                "-owl statue",
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
                "-deku king chamber",
                "-deku king chamber (upper)",
                "-deku shrine",
                "From Southern Swamp (Alternate)",
                "-jp grotto left, first room",
                "-jp grotto left, second room",
                "-jp grotto right, second room",
                "From Bean Seller Grotto",
                "-jp grotto right, first room",
            }
        },
        {
            118, "Deku Palace Royal Chamber", {
                "-deku palace",
                "-deku palace (upper)",
                "-monkey released",
                "-front of king",
            }
        },
        {
            122, "Road to Southern Swamp", {
                "-termina field",
                "-southern swamp",
                "-swamp shooting gallery",
            }
        },
        {
            132, "Southern Swamp (Before Woodfall Temple)", {
                "-road to southern swamp",
                "-boat house",
                "-woodfall",
                "-deku palace",
                "-deku palace (shortcut)",
                "-hags potion shop",
                "-boat ride",
                "-woods of mystery",
                "-swamp spider house",
                "-ikana canyon",
                "-owl statue",
            }
        },
        {
            134, "Woodfall", {
                "-southern swamp",
                "-unknown",
                "-fairy fountain",
                "-unknown",
                "-owl statue",
            }
        },
        {
            158, "Deku Shrine", {
                "-deku palace",
                "-deku palace"
            }
        },
        {
            168, "Swamp Tourist Center", {
                "Entrance",
                "-koume",
                "-tingle's dad",
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
                "-path to goron village (spring)",
                "-unknown",
                "-goron shrine",
                "-lens of truth",
                "-void out",
            }
        },
        {
            148, "Goron Village (Before Snowhead Temple)", {
                "-path to goron village (winter)",
                "-deku flower",
                "-goron shrine",
                "-lens of truth",
                "-void out",
            }
        },
        {
            150, "Goron Graveyard", {
                "-mountain village",
                "-receiving goron mask",
            }
        },
        {
            154, "Mountain Village (Before Snowhead Temple)", {
                "-after snowhead",
                "-mountain smithy",
                "-path to goron village (winter)",
                "-goron graveyard",
                "-path to snowhead",
                "-on ice",
                "-path to mountain village",
                "-unknown",
                "-owl statue",
            }
        },
        {
            174, "Mountain Village (After Snowhead Temple)", {
                "-after snowhead",
                "-mountain smithy",
                "-path to goron village (spring)",
                "-goron graveyard",
                "-path to snowhead",
                "-behind waterfall",
                "-path to mountain village",
                "-after snowhead (cutscene)",
                "-owl statue",
            }
        },
        {
            178, "Snowhead", {
                "-path to snowhead",
                "-snowhead temple",
                "-fairy fountain",
                "-owl statue",
            }
        },
        {
            180, "Road to Goron Village (Before Snowhead Temple)", {
                "-mountain village (winter)",
                "-goron village (winter)",
                "-goron racetrack",
            }
        },
        {
            182, "Road to Goron Village (After Snowhead Temple)", {
                "-mountain village (spring)",
                "-goron village (spring)",
                "-goron racetrack",
            }
        },
        {
            208, "Goron Racetrack", {
                "-path to mountain village",
                "-race start",
                "-race end",
            }
        }
    }},
    { "Great Bay", {
        {
            34, "Pirates' Fortress (Outdoors)", {
                "-exterior pirates fortress",
                "-lower hookshot room",
                "-upper hookshot room",
                "-silver rupee room",
                "-silver rupee room exit",
                "-barrel room",
                "-barrel room exit",
                "-twin barrel room",
                "-twin barrel room exit",
                "-oob near twin barrel",
                "-telescope",
                "-oob hookshot room",
                "-balcony",
                "-upper hookshot room",
            }
        },
        {
            64, "Pirates' Fortress (Indoors)", {
                "-hookshot room",
                "-hookshot room upper",
                "-100 rupee room",
                "-100 rupee room (egg)",
                "-barrel room",
                "-barrel room (egg)",
                "-twin barrel room",
                "-twin barrel room (egg)",
                "-telescope",
                "-outside, underwater",
                "-outside, telescope",
                "-unknown",
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
                "-zora cape",
                "-zora cape (turtle)",
                "-zora shop",
                "-lulu's room",
                "-evan's room",
                "-japa's room",
                "-mikau & tijo's room",
                "-stage",
                "-after rehearsal",
            }
        },
        {
            104, "Great Bay Coast", {
                "-termina field",
                "-zora cape",
                "-void respawn",
                "-pinnacle rock",
                "-fisherman hut",
                "-pirates fortress",
                "-void resapwn (murky water)",
                "-marine lab",
                "-oceanside spider house",
                "-during zora mask",
                "-after zora mask",
                "-owl statue",
                "-thrown out",
                "-after jumping game",
            }
        },
        {
            106, "Zora Cape", {
                "-great bay coast",
                "-zora hall",
                "-zora hall (turtle)",
                "-void respawn",
                "-waterfall",
                "-fairy fountain",
                "-owl statue",
                "-great bay temple",
                "-after great bay temple",
                "-unknown",
            }
        },
        {
            112, "Pirates' Fortress (Entrance)", {
                "-great bay coast",
                "-pirates fortress",
                "-underwater passage",
                "-underwater jet",
                "-kicked out",
                "-hookshot platform",
                "-passage door",
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
                "-zora cape",
                "-race start",
                "-race end",
                "-game won",
            }
        },
        {
            146, "Zora Hall (Room)", {
                "-mikau from zora hall",
                "-japas from zora hall",
                "-lulu from zora hall",
                "-evan from zora hall",
                "-japa after jam session",
                "-zora shop from zora hall",
                "-evan after composing song",
            }
        },
        {
            184, "Gyorg Arena", {
                "-great bay temple",
                "-falling cutscene",
            }
        },
        {
            190, "-great bay (cutscene)", {
                "zora cape",
            }
        }
    }},
    { "Ikana", {
        {
            32, "Ikana Canyon", {
                "-ikana road",
                "-ghost hut",
                "-music box house",
                "-stone tower",
                "-owl statue",
                "-beneath the well",
                "-sakon's hideout",
                "-after stone tower",
                "-ikana castle",
                "-after house opens",
                "-song of storms cave (house open)",
                "-fairy fountain",
                "-secret shrine",
                "-from song of storms cave",
                "-song of storms cave (house closed) ",
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
                "-road to ikana",
                "-grave 1",
                "-grave 2",
                "-grave 3",
                "-dampe's house",
                "-after keeta defeated",
            }
        },
        {
            144, "Beneath the Well", {
                "-ikana canyon",
                "-ikana castle",
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
                "-after minigame",
                "-beat minigame",
            }
        },
        {
            160, "Road to Ikana", {
                "-termina field",
                "-ikana canyon",
                "-ikana graveyard",
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
                "-ikana canyon",
                "-unknown",
                "-stone tower temple",
                "-owl statue",
            }
        },
        {
            172, "Stone Tower (Inverted)", {
                "-after inverting",
                "-stone tower temple",
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
                "-west clock town",
                "-road to southern swamp",
                "-great bay coast",
                "-path to mountain village",
                "-road to ikana",
                "-milk road",
                "-south clock town",
                "-east clock town",
                "-north clock town",
                "-observatory",
                "-observatory (telescope)",
                "-near ikana",
                "-moon crash",
                "-cremia hug",
                "-skullkid cutscene",
                "-west clock town",
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
                "-gorman track (track exit)",
                "-gorman track (main exit)",
                "At Owl Statue",
                "5?",
                "6?",
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
                "-romani ranch",
                "-after race",
            }
        },
        {
            126, "Cucco Shack", {
                "-romani ranch",
                "-after bunny hood",
            }
        },
        {
            206, "Gorman Track", {
                "-milk road",
                "-unknown",
                "-beat minigame",
                "-milk road behind fence",
                "-milk road fence cutscene",
                "-unknown",
                "-start minigame",
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
            46, "-before clock town", {
                "-falling from cliff",
                "-inside clock tower",
                "-transformed to deku",
                "-void respawn",
                "-song of time flashback",
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

