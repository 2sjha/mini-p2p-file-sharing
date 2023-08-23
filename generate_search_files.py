import random as rd

words = [
    "the",
    "of",
    "and",
    "a",
    "to",
    "in",
    "is",
    "you",
    "that",
    "it",
    "he",
    "was",
    "for",
    "on",
    "are",
    "as",
    "with",
    "his",
    "they",
    "I",
    "at",
    "be",
    "this",
    "have",
    "from",
    "or",
    "one",
    "had",
    "by",
    "word",
    "but",
    "not",
    "what",
    "all",
    "were",
    "we",
    "when",
    "your",
    "can",
    "said",
    "there",
    "use",
    "an",
    "each",
    "which",
    "she",
    "do",
    "how",
    "their",
    "if",
    "will",
    "up",
    "other",
    "about",
    "out",
    "many",
    "then",
    "them",
    "these",
    "so",
    "some",
    "her",
    "would",
    "make",
    "like",
    "him",
    "into",
    "time",
    "has",
    "look",
    "two",
    "more",
    "write",
    "go",
    "see",
    "number",
    "no",
    "way",
    "could",
    "people",
    "my",
    "than",
    "first",
    "water",
    "been",
    "call",
    "who",
    "oil",
    "its",
    "now",
    "find",
    "long",
    "down",
    "day",
    "did",
    "get",
    "come",
    "made",
    "may",
    "part",
    "over",
    "new",
    "sound",
    "take",
    "only",
    "little",
    "work",
    "know",
    "place",
    "years",
    "live",
    "me",
    "back",
    "give",
    "most",
    "very",
    "after",
    "thing",
    "our",
    "just",
    "name",
    "good",
    "sentence",
    "man",
    "think",
    "say",
    "great",
    "where",
    "help",
    "through",
    "much",
    "before",
    "line",
    "right",
    "too",
    "mean",
    "old",
    "any",
    "same",
    "tell",
    "boy",
    "follow",
    "came",
    "want",
    "show",
    "also",
    "around",
    "form",
    "three",
    "small",
    "set",
    "put",
    "end",
    "does",
    "another",
    "well",
    "large",
    "must",
    "big",
    "even",
    "such",
    "because",
    "turn",
    "here",
    "why",
    "ask",
    "went",
    "men",
    "read",
    "need",
    "land",
    "different",
    "home",
    "us",
    "move",
    "try",
    "kind",
    "hand",
    "picture",
    "again",
    "change",
    "off",
    "play",
    "spell",
    "air",
    "away",
    "animal",
    "house",
    "point",
    "page",
    "letter",
    "mother",
    "answer",
    "found",
    "study",
    "still",
    "learn",
    "should",
    "America",
    "world",
    "high",
    "every",
    "near",
    "add",
    "food",
    "between",
    "own",
    "below",
    "country",
    "plant",
    "last",
    "school",
    "father",
    "keep",
    "tree",
    "never",
    "start",
    "city",
    "earth",
    "eye",
    "light",
    "thought",
    "head",
    "under",
    "story",
    "saw",
    "left",
    "don't",
    "few",
    "while",
    "along",
    "might",
    "close",
    "something",
    "seem",
    "next",
    "hard",
    "open",
    "example",
    "begin",
    "life",
    "always",
    "those",
    "both",
    "paper",
    "together",
    "got",
    "group",
    "often",
    "run",
    "important",
    "until",
    "children",
    "side",
    "feet",
    "car",
    "mile",
    "night",
    "walk",
    "white",
    "sea",
    "began",
    "grow",
    "took",
    "river",
    "four",
    "carry",
    "state",
    "once",
    "book",
    "hear",
    "stop",
    "without",
    "second",
    "later",
    "miss",
    "idea",
    "enough",
    "eat",
    "face",
    "watch",
    "far",
    "Indian",
    "real",
    "almost",
    "let",
    "above",
    "girl",
    "sometimes",
    "mountains",
    "cut",
    "young",
    "talk",
    "soon",
    "list",
    "song",
    "being",
    "leave",
    "family",
    "it's",
    "body",
    "music",
    "color",
    "stand",
    "sun",
    "questions",
    "fish",
    "area",
    "mark",
    "dog",
    "horse",
    "birds",
    "problem",
    "complete",
    "room",
    "knew",
    "since",
    "ever",
    "piece",
    "told",
    "usually",
    "didn't",
    "friends",
    "easy",
    "heard",
    "order",
    "red",
    "door",
    "sure",
    "become",
    "top",
    "ship",
    "across",
    "today",
    "during",
    "short",
    "better",
    "best",
    "however",
    "low",
    "hours",
    "black",
    "products",
    "happened",
    "whole",
    "measure",
    "remember",
    "early",
    "waves",
    "reached",
    "listen",
    "wind",
    "rock",
    "space",
    "covered",
    "fast",
    "several",
    "hold",
    "himself",
    "toward",
    "five",
    "step",
    "morning",
    "passed",
    "vowel",
    "true",
    "hundred",
    "against",
    "pattern",
    "numeral",
    "table",
    "north",
    "slow",
    "money",
    "map",
    "farm",
    "pulled",
    "draw",
    "voice",
    "seen",
    "cold",
    "cried",
    "plan",
    "notice",
    "south",
    "sing",
    "war",
    "ground",
    "fall",
    "king",
    "town",
    "I'll",
    "unit",
    "figure",
    "certain",
    "field",
    "travel",
    "wood",
    "fire",
    "upon",
    "done",
    "English",
    "road",
    "half",
    "ten",
    "fly",
    "gave",
    "box",
    "finally",
    "wait",
    "correct",
    "oh",
    "quickly",
    "person",
    "became",
    "shown",
    "minutes",
    "strong",
    "verb",
    "stars",
    "front",
    "feel",
    "fact",
    "inches",
    "street",
    "decided",
    "contain",
    "course",
    "surface",
    "produce",
    "building",
    "ocean",
    "class",
    "note",
    "nothing",
    "rest",
    "carefully",
    "scientists",
    "inside",
    "wheels",
    "stay",
    "green",
    "known",
    "island",
    "week",
    "less",
    "machine",
    "base",
    "ago",
    "stood",
    "plane",
    "system",
    "behind",
    "ran",
    "round",
    "boat",
    "game",
    "force",
    "brought",
    "understand",
    "warm",
    "common",
    "bring",
    "explain",
    "dry",
    "though",
    "language",
    "shape",
    "deep",
    "thousands",
    "yes",
    "clear",
    "equation",
    "yet",
    "government",
    "filled",
    "heat",
    "full",
    "hot",
    "check",
    "object",
    "am",
    "rule",
    "among",
    "noun",
    "power",
    "cannot",
    "able",
    "six",
    "size",
    "dark",
    "ball",
    "material",
    "special",
    "heavy",
    "fine",
    "pair",
    "circle",
    "include",
    "built",
    "stay",
    "surface",
    "cover",
    "meaning",
    "reached",
]

search_files = [
    "a.txt",
    "b.txt",
    "c.txt",
    "d.txt",
    "e.txt",
    "f.txt",
    "g.txt",
    "h.txt",
    "i.txt",
    "j.txt",
    "k.txt",
    "l.txt",
    "m.txt",
    "n.txt",
    "o.txt",
    "p.txt",
    "q.txt",
    "r.txt",
    "s.txt",
    "t.txt",
    "u.txt",
    "v.txt",
    "w.txt",
    "x.txt",
    "y.txt",
    "z.txt",
]


def generate_random_search_files():
    path = "./search-files"
    words_len = len(words)
    for search_file in search_files:
        search_file_w = open(path + "/" + search_file, "w")
        for i in range(0, 10):
            for i in range(0, 9):
                rand_word = words[rd.randrange(0, words_len)]
                search_file_w.write(rand_word + " ")
            rand_word = words[rd.randrange(0, words_len)]
            search_file_w.write(rand_word + ".\n")
        search_file_w.close()
        print(search_file + " written successfully.")


if __name__:
    generate_random_search_files()
