#!/usr/bin/env python3
"""Writes 3ds/core/pd_guide_gen.c: the GUIDE's hand-written pages, bosses and
area data for FireRed, LeafGreen and Emerald, read from the app's own sources
so the two never drift:

- GuideFireRed.kt (built twice: leafGreen = false / true) and GuideEmerald.kt
  - pages, sections and entries, their `Have` checks and `areas` tags, the
  bosses (Boss(...) with their done flag and trainer);
- GuideAreas{FireRed,LeafGreen,Emerald}Gen.kt - what each area holds.

The guide files are a small Kotlin DSL (page / section / entry calls, string
concatenation, a ${prizeMons(leafGreen)} template, named arguments), parsed
here with a tiny tokenizer rather than regexes.

    python3 3ds/tools/gen_guide.py
"""
import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(ROOT, "app/src/main/kotlin/com/pokedaisy/app/companion/data")
OUT = os.path.join(ROOT, "3ds/core/pd_guide_gen.c")

HACK_GUIDES = [
    ("gaia", "Gaia", "GAIA"), ("rocket_edition", "RocketEdition", "ROCKET_EDITION"), ("ash_gray", "AshGray", "ASH_GRAY"),
    ("ghost_grey", "GhostGrey", "GHOST_GREY"), ("gs_chronicles", "GsChronicles", "GS_CHRONICLES"),
    ("radical_red", "RadicalRed", "RADICAL_RED"), ("orange_islands", "OrangeIslands", "ORANGE_ISLANDS"),
    ("odyssey", "Odyssey", "ODYSSEY"), ("celia", "Celia", "CELIA"), ("sword_shield", "SwordShield", "SWORD_SHIELD"),
    ("unbound", "Unbound", "UNBOUND"), ("amethyst", "Amethyst", "AMETHYST"), ("soulgold", "SoulGold", "SOULGOLD"),
    ("imperium", "Imperium", "IMPERIUM"), ("quetzal", "Quetzal", "QUETZAL"),
    ("recharged_emerald", "RechargedEmerald", "RECHARGED_EMERALD"), ("recharged_yellow", "RechargedYellow", "RECHARGED_YELLOW"),
]
# Hacks whose guide has no area file in the app (their WHERE IS is empty here).
AREAS_OPTIONAL = {"soulgold", "imperium", "quetzal", "recharged_emerald"}
AREA_KINDS = ["ITEM", "HIDDEN", "GIFT", "KEY", "MON", "EGG", "TRADE"]


def read(name):
    with open(os.path.join(DATA, name), encoding="utf-8") as f:
        return f.read()


# --- a tokenizer and parser for the Kotlin the guides use ---

TOKEN = re.compile(r'\s+|//[^\n]*|/\*.*?\*/|(?P<str>")|(?P<num>0x[0-9A-Fa-f]+|\d+)|(?P<id>[A-Za-z_][A-Za-z0-9_]*)|(?P<p>[(),=+.{}\[\]:<>\-*/])', re.S)


def tokenize(text, pos=0, end=None):
    end = len(text) if end is None else end
    toks = []
    while pos < end:
        m = TOKEN.match(text, pos)
        if not m:
            raise SyntaxError(f"gen_guide: can't read {text[pos:pos + 40]!r}")
        if m.group("str"):
            parts, pos = read_string(text, m.end())
            toks.append(("str", parts))
            continue
        if m.group("num"):
            toks.append(("num", int(m.group("num"), 0)))
        elif m.group("id"):
            toks.append(("id", m.group("id")))
        elif m.group("p"):
            toks.append(("p", m.group("p")))
        pos = m.end()
    return toks


def read_string(text, pos):
    """A "..." literal from just after its quote: a list of str / ('tmpl', expr text) parts."""
    parts, buf = [], ""
    while True:
        c = text[pos]
        if c == "\\":
            esc = text[pos + 1]
            buf += {"n": "\n", "t": "\t", '"': '"', "\\": "\\", "$": "$"}.get(esc, esc)
            pos += 2
        elif c == '"':
            parts.append(buf)
            return parts, pos + 1
        elif c == "$" and re.match(r"[A-Za-z_]", text[pos + 1]):  # $name, a bare template
            name = re.match(r"[A-Za-z_]\w*", text[pos + 1:]).group(0)
            parts.append(buf)
            buf = ""
            parts.append(("tmpl", name))
            pos += 1 + len(name)
        elif text.startswith("${", pos):
            depth, j = 1, pos + 2
            while depth:
                depth += {"{": 1, "}": -1}.get(text[j], 0)
                j += 1
            parts.append(buf)
            buf = ""
            parts.append(("tmpl", text[pos + 2:j - 1]))
            pos = j
        else:
            buf += c
            pos += 1


class Parser:
    """Expressions: calls (positional + named args), string concatenation,
    integer sums of constants, a.b member access."""

    def __init__(self, toks, env):
        self.t, self.i, self.env = toks, 0, env

    def peek(self, k=0):
        return self.t[self.i + k] if self.i + k < len(self.t) else ("eof", None)

    def take(self, kind=None, value=None):
        tok = self.peek()
        if (kind and tok[0] != kind) or (value is not None and tok[1] != value):
            raise SyntaxError(f"gen_guide: expected {kind} {value}, got {tok}")
        self.i += 1
        return tok

    def expr(self):
        left = self.term()
        while self.peek() == ("p", "+"):
            self.take()
            right = self.term()
            left = left + right  # str + str or int + int
        return left

    def term(self):
        kind, value = self.peek()
        if kind == "str":
            self.take()
            return "".join(p if isinstance(p, str) else self.env["template"](p[1]) for p in value)
        if kind == "num":
            self.take()
            return value
        if kind == "id":
            self.take()
            name = value
            while self.peek() == ("p", "."):
                self.take()
                name += "." + self.take("id")[1]
            if self.peek() == ("p", "("):
                return self.call(name)
            if name in self.env["consts"]:
                return self.env["consts"][name]
            if name in self.env.get("strings", {}):  # a guide's own string locals (Ruby / Sapphire's legend)
                return self.env["strings"][name]
            if name in ("true", "false"):
                return name == "true"
            return ("name", name)
        raise SyntaxError(f"gen_guide: unexpected {self.peek()}")

    def call(self, name):
        self.take("p", "(")
        args, kwargs = [], {}
        while self.peek() != ("p", ")"):
            if self.peek()[0] == "id" and self.peek(1) == ("p", "="):
                key = self.take()[1]
                self.take()
                kwargs[key] = self.expr()
            else:
                args.append(self.expr())
            if self.peek() == ("p", ","):
                self.take()
        self.take("p", ")")
        return ("call", name, args, kwargs)


def consts(text):
    return {name: int(v, 0) for name, v in re.findall(r"const val (\w+) = (0x[0-9A-Fa-f]+|\d+)", text)}


def matching_paren(text, open_at):
    depth, i, in_str = 0, open_at, False
    while True:
        c = text[i]
        if in_str:
            if c == "\\":
                i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1


def pages_of(text, env):
    """The `pages = listOf(...)` of a GameGuide, as [(title, [(heading, note, [entry dict])])]."""
    if "pages = emptyList()" in text:
        return []
    start = text.index("pages = listOf(") + len("pages = ")
    end = matching_paren(text, text.index("(", start))
    tree = Parser(tokenize(text, start, end + 1), env).expr()
    pages = []
    for page in tree[2]:
        assert page[1] == "page", page[1]
        title, *sections = page[2]
        out_sections = []
        for sec in sections:
            assert sec[1] == "section", sec[1]
            heading, *entries = sec[2]
            note = sec[3].get("note")
            out_entries = []
            for e in entries:
                assert e[1] == "entry", e[1]
                args, kw = e[2], e[3]
                title_e, answer = args[0], args[1]
                hint = kw.get("hint", args[2] if len(args) > 2 else None)
                have = kw.get("have")
                areas = kw.get("areas")
                have_c = ("NONE", [])
                if have:
                    kind = have[1].split(".")[-1].upper()
                    have_c = (kind, have[2])
                area_list = [a for a in areas[2]] if areas else []
                out_entries.append(dict(title=title_e, answer=answer, hint=hint, have=have_c, areas=area_list))
            out_sections.append((heading, note, out_entries))
        pages.append((title, out_sections))
    return pages


def bosses_of(text, decl, env):
    """Boss(title, where, doneFlag) { id } / trainer = league(a, b) / the champion's rival lambda."""
    body = text[text.index(f"val {decl} = listOf("):]
    body = body[:matching_paren(body, body.index("(")) + 1]
    out = []
    named = list(re.finditer(r'Boss\(("[^"]*"), ("[^"]*"), ([^,){]+?), trainer = ([a-z]\w*)\)', body))
    for m in named:
        # trainer = brawly: `private val brawly: (SaveProgress) -> Int = { p -> when { p.flag(F) -> N ... else -> M } }`
        lam = text[text.index(f"val {m.group(4)}:"):]
        lam = lam[:lam.index("\n}\n") + 2]
        alts = [(Parser(tokenize(f), env).expr(), int(n)) for f, n in re.findall(r"p\.flag\(([^)]+)\) -> (\d+)", lam)]
        default = int(re.search(r"else -> (\d+)", lam).group(1))
        title = Parser(tokenize(m.group(1)), env).expr()
        where = Parser(tokenize(m.group(2)), env).expr()
        flag = Parser(tokenize(m.group(3)), env).expr()
        out.append((m.start(), (title, where, flag, "BY_FLAGS", default, 0, alts, [], [])))
    plain = []
    for m in re.finditer(r'Boss\(("[^"]*"), ("[^"]*"), ([^,){]+?)(?:, doneAny = listOf\(([^)]*)\))?(?:, variants = (normalHard\(\d+, \d+\)|listOf\((?:"[^"]*" to \d+(?:, )?)+\)))?(?:, trainer = league\((\d+), (\d+)\))?\)\s*(\{[^}]*\})?', body):
        title = Parser(tokenize(m.group(1)), env).expr()
        where = Parser(tokenize(m.group(2)), env).expr()
        flag = Parser(tokenize(m.group(3)), env).expr()
        done_any = [Parser(tokenize(e.strip()), env).expr() for e in m.group(4).split(",")] if m.group(4) else []
        variants = []
        if m.group(5):
            v = m.group(5)
            if v.startswith("normalHard"):
                n, h = re.findall(r"\d+", v)
                variants = [("NORMAL", int(n)), ("HARD", int(h))]
            else:
                variants = [(l, int(n)) for l, n in re.findall(r'"([^"]*)" to (\d+)', v)]
        if m.group(6):
            plain.append((m.start(), (title, where, flag, "LEAGUE", int(m.group(6)), int(m.group(7)), [], variants, done_any)))
        else:
            lam = m.group(8) or ""
            champ = re.search(r"league\((\d+), (\d+)\)\(p\) \+ rivalOffset", body[m.end():m.end() + 300])
            if "rivalOffset" in lam or (champ and "val rivalOffset" in body[m.end():m.end() + 200]):
                plain.append((m.start(), (title, where, flag, "CHAMPION", int(champ.group(1)), int(champ.group(2)), [], variants, done_any)))
            else:
                plain.append((m.start(), (title, where, flag, "FIXED", int(re.search(r"\d+", lam).group()), 0, [], variants, done_any)))
    # In the list's order; every Boss( read, or the guide would quietly skip one.
    bosses = [b for _, b in sorted(out + plain, key=lambda e: e[0])]
    assert len(bosses) == body.count("Boss("), f"gen_guide: {decl}: read {len(bosses)} of {body.count('Boss(')} bosses"
    return bosses


def areas_of(name, decl):
    text = read(name)
    body = text[text.index(f"val {decl}"):]
    things = []
    for line in re.findall(r"^(\d+\|[A-Z]+\|.*)$", body, re.M):
        mapsec, kind, id_, flag, qty, wants, where = line.split("|", 6)
        things.append((int(mapsec), kind, int(id_), int(flag), int(qty), int(wants), where))
    things.sort(key=lambda t: t[0])
    return things


def c_str(s):
    if s is None:
        return "NULL"
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def area_key(name):
    """The app's areaKey(): upper case, É -> E, letters and digits only."""
    return "".join(ch for ch in name.upper().replace("É", "E") if ch.isalnum())


def emit_guide(out, prefix, pages, bosses, verified, extra):
    for pi, (title, sections) in enumerate(pages):
        for si, (heading, note, entries) in enumerate(sections):
            out.append(f"static const struct pd_guide_entry {prefix}_p{pi}_s{si}[] = {{")
            for e in entries:
                kind, vals = e["have"]
                vals = (vals + [0, 0, 0])[:3]
                areas = "|".join(area_key(a) for a in e["areas"])
                out.append(f"    {{ {c_str(e['title'])}, {c_str(e['answer'])}, {c_str(e['hint'])}, "
                           f"PD_HAVE_{kind}, {{ {vals[0]}, {vals[1]}, {vals[2]} }}, {c_str(areas or None)} }},")
            out.append("};")
        out.append(f"static const struct pd_guide_section {prefix}_p{pi}[] = {{")
        for si, (heading, note, entries) in enumerate(sections):
            out.append(f"    {{ {c_str(heading)}, {c_str(note)}, {prefix}_p{pi}_s{si}, {len(entries)} }},")
        out.append("};")
    out.append(f"static const struct pd_guide_page {prefix}_pages[] = {{")
    for pi, (title, sections) in enumerate(pages):
        out.append(f"    {{ {c_str(title)}, {prefix}_p{pi}, {len(sections)} }},")
    out.append("};")
    for bi, (title, where, flag, kind, a, b, alts, variants, done_any) in enumerate(bosses):
        if variants:
            vs = ", ".join(f"{{ {c_str(l)}, {t} }}" for l, t in variants)
            out.append(f"static const struct pd_variant {prefix}_vars{bi}[] = {{ {vs}, {{ NULL, 0 }} }};")
        if done_any:
            out.append(f"static const uint16_t {prefix}_any{bi}[] = {{ {', '.join(f'0x{f:X}' for f in done_any)}, 0 }};")
        if alts:
            pairs = ", ".join(f"0x{f:X}, {t}" for f, t in alts)
            out.append(f"static const uint16_t {prefix}_alts{bi}[] = {{ {pairs}, 0 }};")
    out.append(f"static const struct pd_boss {prefix}_bosses[] = {{")
    for bi, (title, where, flag, kind, a, b, alts, variants, done_any) in enumerate(bosses):
        alt = f"{prefix}_alts{bi}" if alts else "NULL"
        var = f"{prefix}_vars{bi}" if variants else "NULL"
        anyf = f"{prefix}_any{bi}" if done_any else "NULL"
        out.append(f"    {{ {c_str(title)}, {c_str(where)}, 0x{flag:X}, PD_BOSS_{kind}, {a}, {b}, {alt}, {var}, {anyf} }},")
    out.append("};")
    out.append(f"const struct pd_guide {prefix} = {{")
    out.append(f"    {prefix}_pages, {len(pages)}, {prefix}_bosses, {len(bosses)}, {str(verified).lower()}, {extra},")
    out.append("};")
    out.append("")


def emit_areas(out, prefix, things):
    out.append(f"const struct pd_area_thing {prefix}[] = {{")
    for mapsec, kind, id_, flag, qty, wants, where in things:
        out.append(f"    {{ {mapsec}, PD_AREA_{kind}, {id_}, {flag}, {qty}, {wants}, {c_str(where)} }},")
    out.append("};")
    out.append(f"const int {prefix}_count = {len(things)};")
    out.append("")


def expand_helpers(slug, text):
    """Rewrite a hack's boss-building helper calls into plain Boss(...) lines (what bosses_of reads)."""
    if slug == "unbound":  # gym(n, title, where, difficult, vanilla, expert, insane) -> a Boss with its four teams
        text = re.sub(r'gym\((\d+), ("[^"]*"), ("[^"]*"), (\d+), (\d+), (\d+), (\d+)\)',
                      lambda m: f'Boss({m.group(2)}, {m.group(3)}, FLAG_BADGE01_GET + {int(m.group(1)) - 1}, variants = listOf("VANILLA" to {m.group(5)}, "DIFFICULT" to {m.group(4)}, "EXPERT" to {m.group(6)}, "INSANE" to {m.group(7)})) {{ {m.group(4)} }}',
                      text.replace("private fun gym(", "private fun gym_unused("))
    elif slug == "gaia":  # elite(n, title, trainer): done by its own flag or once the game is cleared
        text = re.sub(r'elite\((\d+), ("[^"]*"), (\d+)\)',
                      lambda m: f'Boss({m.group(2)}, "POKéMON LEAGUE", FLAG_DEFEATED_E4_1 + {m.group(1)}, doneAny = listOf(FLAG_SYS_GAME_CLEAR)) {{ {m.group(3)} }}',
                      text.replace("private fun elite(", "private fun elite_unused("))
    elif slug == "imperium":  # elite(n, title, "LABEL" to id, ...): done by its own flag or once champion
        text = re.sub(r'(?m)^    elite\((\d+), ("[^"]*"), ((?:"[^"]*" to \d+(?:, )?)+)\),',
                      lambda m: f'    Boss({m.group(2)}, "POKéMON LEAGUE", 0, doneAny = listOf(FLAG_DEFEATED_ELITE_4_SIDNEY + {m.group(1)}, FLAG_IS_CHAMPION), variants = listOf({m.group(3)})) {{ {re.search(r"to (\d+)", m.group(3)).group(1)} }},',
                      text)
    elif slug in ("recharged_emerald", "recharged_yellow"):  # gym / elite / league helpers; HARD = the record's second party
        hard = lambda i: int(i) + 65536
        nh = lambda i: f'listOf("NORMAL" to {i}, "HARD" to {hard(i)})'
        text = re.sub(r'(?m)^    gym\((\d+), ("[^"]*"), ("[^"]*"), (\d+)\),',
                      lambda m: f'    Boss({m.group(2)}, {m.group(3)}, FLAG_BADGE01_GET + {m.group(1)}, variants = {nh(m.group(4))}) {{ {m.group(4)} }},', text)
        text = re.sub(r'(?m)^    elite\((\d+), ("[^"]*"), (\d+)\),',
                      lambda m: f'    Boss({m.group(2)}, "POKéMON LEAGUE", FLAG_DEFEATED_ELITE_4_SIDNEY + {m.group(1)}, doneAny = listOf(FLAG_IS_CHAMPION), variants = {nh(m.group(3))}) {{ {m.group(3)} }},', text)
        text = re.sub(r'Boss\(\s*"CHAMPION WALLACE", "POKéMON LEAGUE", FLAG_IS_CHAMPION, variants = nh\((\d+)\),\s*doneIf = [^\n]*\n\s*\) \{ \d+ \},',
                      lambda m: f'Boss("CHAMPION WALLACE", "POKéMON LEAGUE", FLAG_IS_CHAMPION, doneAny = listOf(FLAG_SYS_GAME_CLEAR), variants = {nh(m.group(1))}) {{ {m.group(1)} }},', text)
        text = re.sub(r'(?m)^    league\((\d+), ("[^"]*"), (\d+), (\d+)\),',
                      lambda m: f'    Boss({m.group(2)}, "POKéMON LEAGUE", FLAG_DEFEATED_LORELEI + {m.group(1)}, trainer = league({m.group(3)}, {m.group(4)})) {{ {m.group(3)} }},', text)
    elif slug == "celia":  # BLAINE's eight one-on-one battles
        text = text.replace('(1..8).map { "BATTLE $it" to 502 + it }',
                            "listOf(" + ", ".join(f'"BATTLE {i}" to {502 + i}' for i in range(1, 9)) + ")")
    elif slug == "ghost_grey":  # SUMMER's team answers the starter: all three are shown, done when any is beaten
        text = re.sub(r'Boss\(\s*"BEST FRIEND SUMMER", "FUJI CATACOMBS", FLAG_BADGE01_GET \+ 7,.*?\n    \) \{ p -> when.*?\},\n',
                      'Boss("BEST FRIEND SUMMER", "FUJI CATACOMBS", TRAINER_FLAGS_START + 435, '
                      'doneAny = listOf(TRAINER_FLAGS_START + 436, TRAINER_FLAGS_START + 437), '
                      'variants = listOf("LITWICK" to 435, "SPIRITLE" to 436, "ROWLET" to 437)) { 437 },\n', text, flags=re.S)
    return text


# --- Game Boy (Gen 1): GuideGen1Gen.kt's boss lines -> teams + bosses (RAM flags are packed into one array, see below) ---
GB_GAMES = [("yellow", "G1Y"), ("red", "G1R"), ("blue", "G1B")]
# The snapshot's flags array for a Gen 1 game: event flags, then the missable-object, hidden-item, trade and badge banks.
GB_BANK_BIT = {0: 0, 1: 320 * 8, 2: 320 * 8 + 256, 3: 320 * 8 + 256 + 112, 4: 320 * 8 + 256 + 112 + 16}


def gb_flag(f):
    return GB_BANK_BIT[f >> 16] + (f & 0xFFFF)


def emit_gb(out):
    text = read("GuideGen1Gen.kt")
    for slug, pre in GB_GAMES:
        m = re.search(rf'val {pre}_BOSSES_RAW: List<String> = listOf\(\s*"""(.*?)"""', text, re.S)
        lines = [l for l in m.group(1).split("\n") if l.strip()]
        teams, bosses = [], []  # bosses: [key, title, where, done, [(var or 0, team index)]]
        for l in lines:
            key, title, where, done, cond, mons = l.split("|")
            var = int(cond.split("=")[1]) if cond else -1
            ms = []
            for mon in [x for x in mons.split(";") if x.strip()]:
                p = mon.split(" ")
                moves = [int(x) for x in (p[3].split(",") if len(p) > 3 else []) if x]
                ms.append((int(p[0]), int(p[1]), int(p[2]), (moves + [0, 0, 0, 0])[:4]))
            assert 1 <= len(ms) <= 6, l
            teams.append((var, ms))
            if bosses and bosses[-1][0] == key:
                bosses[-1][4].append((var, len(teams) - 1))
            else:
                bosses.append([key, title, where, [gb_flag(int(x, 0)) for x in done.split("/") if x], [(var, len(teams) - 1)]])
        out.append(f"const struct pd_gb_team pd_gb_teams_{slug}[] = {{")
        for var, ms in teams:
            mm = ", ".join(f"{{ {sp}, {it}, {lv}, {{ {', '.join(map(str, mv))} }} }}" for lv, sp, it, mv in ms)
            out.append(f"    {{ {var}, {len(ms)}, {{ {mm} }} }},")
        out.append("    { 0, 0, { { 0, 0, 0, { 0, 0, 0, 0 } } } },")
        out.append("};")
        for bi, (key, title, where, done, ts) in enumerate(bosses):
            if len(ts) > 1:
                vs = ", ".join(f'{{ "", {ti} }}' for _, ti in ts)
                out.append(f"static const struct pd_variant pd_gb_{slug}_vars{bi}[] = {{ {vs}, {{ NULL, 0 }} }};")
                al = ", ".join(f"{v}, {ti}" for v, ti in ts)
                out.append(f"static const uint16_t pd_gb_{slug}_alts{bi}[] = {{ {al}, 0 }};")
            if len(done) > 1:
                out.append(f"static const uint16_t pd_gb_{slug}_any{bi}[] = {{ {', '.join(map(str, done[1:]))}, 0 }};")
        out.append(f"static const struct pd_boss pd_gb_{slug}_bosses[] = {{")
        for bi, (key, title, where, done, ts) in enumerate(bosses):
            multi = len(ts) > 1
            kind = "PD_BOSS_TEAMVAR" if multi else "PD_BOSS_FIXED"
            alts = f"pd_gb_{slug}_alts{bi}" if multi else "NULL"
            var = f"pd_gb_{slug}_vars{bi}" if multi else "NULL"
            anyf = f"pd_gb_{slug}_any{bi}" if len(done) > 1 else "NULL"
            out.append(f"    {{ {c_str(title)}, {c_str(where)}, {done[0]}, {kind}, {ts[0][1]}, 0, {alts}, {var}, {anyf} }},")
        out.append("};")
        out.append(f"const struct pd_guide pd_guide_gb_{slug} = {{ NULL, 0, pd_gb_{slug}_bosses, {len(bosses)}, false, 0, 0 }};")
        out.append("")


def main():
    fr_text = read("GuideFireRed.kt")
    em_text = read("GuideEmerald.kt")
    fr_consts = consts(fr_text)
    em_consts = consts(em_text)

    # ${prizeMons(leafGreen)}: the one template, `if (leafGreen) { "LG" } else { "FR" }`.
    prize = re.search(r'fun prizeMons\(leafGreen: Boolean\) = if \(leafGreen\) \{\s*"([^"]*)"\s*\} else \{\s*"([^"]*)"', fr_text)

    def fr_env(leaf_green):
        def template(expr):
            if expr.strip() == "prizeMons(leafGreen)":
                return prize.group(1) if leaf_green else prize.group(2)
            raise SyntaxError(f"gen_guide: unknown template {expr}")
        return {"consts": fr_consts, "template": template}

    def no_template(expr):
        raise SyntaxError(f"gen_guide: unknown template {expr}")

    em_env = {"consts": em_consts, "template": no_template}

    # Ruby and Sapphire: one text built per version (rubySapphireGuide(ruby)), its locals by version.
    rs_text = read("GuideRubySapphire.kt")
    rs_consts = consts(rs_text)

    def rs_env(ruby):
        local = {"team": "TEAM MAGMA" if ruby else "TEAM AQUA", "legend": "GROUDON" if ruby else "KYOGRE",
                 "roamer": "LATIOS" if ruby else "LATIAS"}
        nums = dict(rs_consts, legendNo=383 if ruby else 382, roamerNo=381 if ruby else 380)

        def template(expr):
            if expr.strip() in local:
                return local[expr.strip()]
            raise SyntaxError(f"gen_guide: unknown template {expr}")
        return {"consts": nums, "template": template, "strings": local}

    rs_bosses = bosses_of(rs_text, "BOSSES_RUBY_SAPPHIRE", rs_env(True))

    # Emerald Legacy: its own bosses (BRAWLY by flags), no hand-written WHERE IS (built from its areas at run time).
    el_text = read("GuideEmeraldLegacy.kt")
    el_env = {"consts": consts(el_text), "template": no_template}
    el_bosses = bosses_of(el_text, "BOSSES_EMERALD_LEGACY", el_env)

    # The League's rematch flag and the starter var (the champion's team).
    league_flag = fr_consts["FLAG_SYS_CAN_LINK_WITH_RS"]
    starter_var = fr_consts["VAR_STARTER_MON"]
    fr_bosses = bosses_of(fr_text, "BOSSES_FIRERED", fr_env(False))
    em_bosses = bosses_of(em_text, "BOSSES_EMERALD", em_env)
    em_verified = "verified = false" not in em_text

    # The hacks whose guide is the same DSL with plain bosses (no NORMAL/HARD variants, no custom lambdas): slug, Kotlin
    # name, constant name.
    hack_guides = []
    for slug, cls, up in HACK_GUIDES:
        text = read(f"Guide{cls}.kt")
        env = {"consts": consts(text), "template": no_template}
        try:
            text = expand_helpers(slug, text)
            env = {"consts": consts(text), "template": no_template}
            bosses = bosses_of(text, f"BOSSES_{up}", env)
            lst = text[text.index(f"val BOSSES_{up}"):]
            lst = lst[lst.index("\n"):lst.index("\n)\n")]
            helpers = re.findall(r"^\s+(?!Boss\()([a-z]\w*)\(", lst, re.M)
            assert not helpers, f"bosses built by helper {helpers[0]}()"
            pages_of(text, env)
        except (AssertionError, SyntaxError, KeyError, ValueError) as e:
            print(f"gen_guide: {slug} left out: {e}")
            continue
        hack_guides.append((slug, cls, up, text, env, bosses))

    out = [
        "// Generated by 3ds/tools/gen_guide.py from the app's GuideFireRed.kt, GuideEmerald.kt, GuideRubySapphire.kt, GuideEmeraldLegacy.kt",
        "// and GuideAreas{FireRed,LeafGreen,Emerald,RubySapphire,EmeraldLegacy}Gen.kt - do not hand-edit.",
        '#include "pd_guide.h"',
        "",
    ]
    emit_guide(out, "pd_guide_firered", pages_of(fr_text, fr_env(False)), fr_bosses, True,
               f"0x{league_flag:X}, 0x{starter_var:X}")
    emit_guide(out, "pd_guide_leafgreen", pages_of(fr_text, fr_env(True)), fr_bosses, True,
               f"0x{league_flag:X}, 0x{starter_var:X}")
    emit_guide(out, "pd_guide_emerald", pages_of(em_text, em_env), em_bosses, em_verified, "0, 0")
    emit_guide(out, "pd_guide_ruby", pages_of(rs_text, rs_env(True)), rs_bosses,
               "verified = false" not in rs_text, "0, 0")
    emit_guide(out, "pd_guide_sapphire", pages_of(rs_text, rs_env(False)), rs_bosses,
               "verified = false" not in rs_text, "0, 0")
    emit_guide(out, "pd_guide_emerald_legacy", pages_of(el_text, el_env), el_bosses,
               "verified = false" not in el_text, "0, 0")
    for slug, cls, up, text, env, bosses in hack_guides:
        # Recharged Yellow's League rematch teams come once the game is cleared (its own flag, no starter var).
        extra = f"0x{env['consts']['FLAG_SYS_GAME_CLEAR']:X}, 0" if slug == "recharged_yellow" else "0, 0"
        emit_guide(out, f"pd_guide_{slug}", pages_of(text, env), bosses, "verified = false" not in text, extra)
        if os.path.exists(os.path.join(DATA, f"GuideAreas{cls}Gen.kt")):
            emit_areas(out, f"pd_areas_{slug}", areas_of(f"GuideAreas{cls}Gen.kt", f"GUIDE_AREAS_{up}_RAW"))
        else:
            assert slug in AREAS_OPTIONAL, slug
    emit_gb(out)
    emit_areas(out, "pd_areas_emerald_legacy", areas_of("GuideAreasEmeraldLegacyGen.kt", "GUIDE_AREAS_EMERALD_LEGACY_RAW"))
    emit_areas(out, "pd_areas_ruby_sapphire", areas_of("GuideAreasRubySapphireGen.kt", "GUIDE_AREAS_RUBY_SAPPHIRE_RAW"))
    emit_areas(out, "pd_areas_firered", areas_of("GuideAreasFireRedGen.kt", "GUIDE_AREAS_FIRERED_RAW"))
    emit_areas(out, "pd_areas_leafgreen", areas_of("GuideAreasLeafGreenGen.kt", "GUIDE_AREAS_LEAFGREEN_RAW"))
    emit_areas(out, "pd_areas_emerald", areas_of("GuideAreasEmeraldGen.kt", "GUIDE_AREAS_EMERALD_RAW"))

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print(f"wrote {os.path.relpath(OUT, ROOT)}: FR {len(fr_bosses)} bosses, EM {len(em_bosses)} bosses")


if __name__ == "__main__":
    main()
