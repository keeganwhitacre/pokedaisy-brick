/* GPL-3.0. Adapted from BattleInfoScreen, SuggestionsScreen.rankSuggestions,
 * TypeChartUtil and Gen2Reader. This is read-only advice, not a damage
 * calculator. */
void pc_ui_observe(struct pc_ui *u, const struct pd_snapshot *s) {
  if (!s->inBattle) {
    u->battleControls = u->battleParty = false;
    u->wasBattle = false;
    u->battleSeen = 0;
    u->battleFoe = 0;
    return;
  }
  if (!u->wasBattle) {
    u->battleSeen = 0;
    u->battleFoe = 0;
    u->battleRow = 0;
    u->battlePage = 0;
    u->wasBattle = true;
  }
  if (s->foeActive >= 0 && s->foeActive < s->foeCount)
    u->battleSeen |= 1u << s->foeActive;
}
/* BattleControlsScreen.Gen1ActionButtons / Gen1MoveList / PartyPicker:
 * source labels, 2x2 action/move cells and existing party grid. Physical
 * controller focus replaces touch; original game remains authoritative. */
static int battle_pct(int, int, int);
static const char *battle_verdict(int, int);
static void battle_move(int, int, const char **, int *, int *);
static bool pc_battle_controls_input(struct pc_ui *u,
                                     const struct pd_snapshot *s,
                                     enum pc_action a) {
  bool party = u->battleParty || u->battleMenu.state == PB_PARTY ||
               u->battleMenu.state == PB_SUBMENU;
  if (a == PC_BACK) {
    if (party && u->battleMenu.state == PB_ACTION)
      u->battleParty = false;
    else if (u->battleMenu.state == PB_MOVE || (party && !u->battleMenu.forced))
      u->battleRequest = PB_BACK;
    else
      u->battleControls = false;
    return true;
  }
  if (a == PC_ALTERNATE) {
    u->battleControls = false;
    return true;
  }
  if (a == PC_LEFT)
    u->battleChoice &= ~1;
  if (a == PC_RIGHT)
    if (!party || (u->battleChoice | 1) < s->partyCount)
      u->battleChoice |= 1;
  if (a == PC_UP)
    u->battleChoice = party ? (u->battleChoice + 4) % 6 : u->battleChoice & ~2;
  if (a == PC_DOWN)
    u->battleChoice = party ? (u->battleChoice + 2) % 6 : u->battleChoice | 2;
  if (party && u->battleChoice >= s->partyCount)
    u->battleChoice = s->partyCount > 1 ? (u->battleChoice & 1) : 0;
  if (a == PC_CONFIRM) {
    if (party) {
      int i = u->battleChoice;
      if (i < s->partyCount && s->party[i].hp > 0 &&
          s->party[i].species != 255 && i != s->partyActive) {
        u->battleRequest = PB_SWITCH;
        u->battleValue = i;
      }
    } else if (u->battleMenu.state == PB_ACTION) {
      if (u->battleChoice == 1) {
        u->battleParty = true;
        u->battleChoice = 0;
      } else {
        u->battleRequest = PB_CHOOSE_ACTION;
        u->battleValue = (int[]){0, 2, 1, 3}[u->battleChoice];
      }
    } else if (u->battleMenu.state == PB_MOVE) {
      const struct pd_battle_mon *m = &s->battlers[0];
      int i = u->battleChoice;
      if (m->moves[i] && m->pp[i] > 0) {
        u->battleRequest = PB_CHOOSE_MOVE;
        u->battleValue = i;
      }
    }
  }
  return true;
}
static void pc_battle_controls_draw(struct pc_ui *u, struct pd_canvas *c,
                                    const struct pd_snapshot *s,
                                    const unsigned char *rom, size_t n,
                                    int build) {
  header(c, PC_BATTLE);
  bool party = u->battleParty || u->battleMenu.state == PB_PARTY ||
               u->battleMenu.state == PB_SUBMENU;
  if (party) {
    pc_party_draw(c, s, u->battleChoice, false, rom, n, build,
                  ((u->animationMs / 167) % 2) ? 10 : 2);
    pd_fill(c, 0, 338, 512, 46, 0xffffff);
    if (s->partyActive >= 0 && s->partyActive < s->partyCount)
      txt(c, 8 + (s->partyActive % 2) * 250 + 182,
          34 + (s->partyActive / 2) * 100, "OUT");
    txt(c, 12, 346, "Choose a POKéMON. A:Send in");
    txt(c, 12, 366, "B:Cancel Y:Info X:Game");
    return;
  }
  txt(c, 12, 38, "BATTLE CONTROLS  Y:Info");
  if (u->battleMenu.state != PB_ACTION && u->battleMenu.state != PB_MOVE) {
    wrapped(c, 24, 112, 464, 5,
            "Waiting for your turn... Return to the game to advance dialogue.");
  } else
    for (int i = 0; i < 4; i++) {
      int x = 8 + (i & 1) * 252, y = 72 + (i / 2) * 126;
      pd_layered_box(c, x, y, 244, 118, 6, pcOptionFrame, 5, 0xffffff);
      bool enabled = true;
      if (i == u->battleChoice)
        pd_fill(c, x + 18, y + 20, 4, 76, 0x181818);
      if (u->battleMenu.state == PB_ACTION) {
        const char *labels[] = {"Fight", "PkMn", "Bag", "Run"};
        txt(c, x + 34, y + 46, labels[i]);
        if (!i)
          icon(c, x + 170, y + 36, s->battlers[0].species, rom, n, build);
      } else {
        const struct pd_battle_mon *m = &s->battlers[0];
        int id = m->moves[i], type, power;
        const char *name;
        battle_move(id, build, &name, &type, &power);
        enabled = id && m->pp[i] > 0;
        pd_text_fit(c, x + 28, y + 20, 196, name, enabled ? 0x181818 : 0x989898,
                    PD_NO_SHADOW);
        if (id) {
          char b[80];
          snprintf(b, sizeof b, "%s", typeNames[type]);
          pd_text_fit(c, x + 28, y + 48, 196, b, 0x181818, PD_NO_SHADOW);
          int pct =
              battle_pct(type, s->battlers[1].type1, s->battlers[1].type2);
          if (power)
            snprintf(b, sizeof b, "PP %d  %gx", m->pp[i], pct / 100.0);
          else
            snprintf(b, sizeof b, "PP %d", m->pp[i]);
          pd_text_fit(c, x + 28, y + 74, 196, b, 0x181818, PD_NO_SHADOW);
        }
      }
      if (!enabled)
        pocket_fade_rect(c, x + 12, y + 12, 220, 94, 0xffffff, 55);
    }
  txt(c, 12, 346, "D-pad:Pick A:Use B:Back");
  txt(c, 12, 366, "Start:Info X:Game");
}
static int battle_pct(int type, int d1, int d2) {
  int pct = type_percent(type, d1);
  return d1 == d2 ? pct : pct * type_percent(type, d2) / 100;
}
/* TypeChartUtil categories; use live battler types, including changed types. */
static int battle_matchups(const struct pd_battle_mon *foe, int group,
                           int ids[18], int values[18]) {
  int count = 0;
  if (!foe->species)
    return 0;
  for (int t = 0; t < 18; t++) {
    int pct = battle_pct(t, foe->type1, foe->type2);
    if ((group == 0 && pct > 100) || (group == 1 && pct > 0 && pct < 100) ||
        (group == 2 && !pct)) {
      int at = count++;
      while (at > 0 &&
             (group == 0 ? values[at - 1] < pct : values[at - 1] > pct)) {
        ids[at] = ids[at - 1];
        values[at] = values[at - 1];
        at--;
      }
      ids[at] = t;
      values[at] = pct;
    }
  }
  return count;
}
static const char *battle_verdict(int power, int pct) {
  return !power      ? "STATUS"
         : !pct      ? "IMMUNE"
         : pct > 100 ? "SUPER"
         : pct < 100 ? "RESISTED"
                     : "NEUTRAL";
}
static void battle_move(int id, int build, const char **name, int *type,
                        int *power) {
  *name = "-";
  *type = 0;
  *power = 0;
  if (id <= 0 || id >= 256 || !moves[id].name)
    return;
  bool faithful =
      build_ok(build) && builds[build].faithful && faithfulMoves[id].name;
  *name = faithful ? faithfulMoves[id].name : moves[id].name;
  *type = faithful ? faithfulMoves[id].type : moves[id].type;
  *power = faithful ? faithfulMoves[id].power : moves[id].power;
}
struct battle_pick {
  int slot, move, pct, risk, power, score;
};
static int battle_rank(const struct pd_snapshot *s,
                       const struct pd_battle_mon *foe, int build,
                       struct battle_pick picks[6]) {
  if (!foe->species)
    return 0;
  int count = 0;
  for (int i = 0; i < s->partyCount; i++) {
    const struct pd_mon *m = &s->party[i];
    if (m->hp <= 0 || m->species == 255)
      continue;
    int best = -1, highest = -1, pct = 100, power = 0;
    for (int j = 0; j < 4; j++) {
      const char *name;
      int t, p;
      battle_move(m->moves[j], build, &name, &t, &p);
      if (!p || m->pp[j] <= 0)
        continue;
      int multiplier = battle_pct(t, foe->type1, foe->type2);
      if (p * multiplier > highest) {
        best = j;
        highest = p * multiplier;
        pct = multiplier;
        power = p;
      }
    }
    if (best < 0)
      continue;
    const int *types = mon_types(m->species, build);
    int risk = battle_pct(foe->type1, types[1], types[2]);
    int other = battle_pct(foe->type2, types[1], types[2]);
    if (other > risk)
      risk = other;
    picks[count++] = (struct battle_pick){
        i, best, pct, risk, power, power * pct / 100 - risk};
  }
  for (int i = 1; i < count; i++) {
    struct battle_pick p = picks[i];
    int j = i;
    while (j && picks[j - 1].score < p.score) {
      picks[j] = picks[j - 1];
      j--;
    }
    picks[j] = p;
  }
  return count;
}
static struct pd_battle_mon
battle_target(const struct pc_ui *u, const struct pd_snapshot *s, int build) {
  int i = u->battleFoe - 1;
  if (i < 0 || i >= s->foeCount || i == s->foeActive)
    return s->battlers[PD_POS_OPPONENT_LEFT];
  const struct pd_mon *m = &s->foes[i];
  const int *t = mon_types(m->species, build);
  struct pd_battle_mon b = {.species = m->species,
                            .level = m->level,
                            .hp = m->hp,
                            .maxHp = m->maxHp,
                            .status = m->status,
                            .heldItem = m->heldItem,
                            .type1 = t[1],
                            .type2 = t[2]};
  memcpy(b.moves, m->moves, sizeof b.moves);
  memcpy(b.pp, m->pp, sizeof b.pp);
  return b;
}
static void battle_moves_draw(struct pd_canvas *c,
                              const struct pd_battle_mon *m,
                              const struct pd_battle_mon *def, int build) {
  char b[128];
  for (int i = 0; i < 4; i++) {
    int y = 132 + 50 * i, t, p;
    const char *name;
    battle_move(m->moves[i], build, &name, &t, &p);
    pd_fill(c, 10, y - 2, 492, 48, i % 2 ? 0xffffff : 0xececec);
    pd_text_fit(c, 20, y, 330, name, 0x181818, PD_NO_SHADOW);
    snprintf(b, sizeof b, "%dPP", m->pp[i]);
    pd_text_right(c, 490, y, b, 0x181818, PD_NO_SHADOW);
    if (m->moves[i]) {
      int pct = battle_pct(t, def->type1, def->type2);
      if (p)
        snprintf(b, sizeof b, "%s  %s x%s", typeNames[t],
                 battle_verdict(p, pct),
                 pct == 400   ? "4"
                 : pct == 200 ? "2"
                 : pct == 100 ? "1"
                 : pct == 50  ? "1/2"
                 : pct == 25  ? "1/4"
                              : "0");
      else
        snprintf(b, sizeof b, "%s  STATUS", typeNames[t]);
      txt(c, 20, y + 22, b);
    }
  }
}
static void pc_battle_pages(struct pc_ui *u, struct pd_canvas *c,
                            const struct pd_snapshot *s,
                            const unsigned char *rom, size_t n, int build) {
  if (!s->inBattle) {
    pc_battle_draw(c, s, rom, n, build);
    return;
  }
  header(c, PC_BATTLE);
  const char *pages[] = {"INFO", "SUGGESTIONS", "FOE DETAILS", "FOE TEAM",
                         "FOE TYPES"};
  char b[256];
  snprintf(b, sizeof b, "%s  Y:Page", pages[u->battlePage]);
  txt(c, 12, 36, b);
  struct pd_battle_mon target = battle_target(u, s, build);
  const struct pd_battle_mon *player = &s->battlers[PD_POS_PLAYER_LEFT];
  if (u->battleFoe > s->foeCount)
    u->battleFoe = 0;
  if (u->battlePage == 3) {
    if (!s->isTrainer || !s->foeCount)
      txt(c, 20, 94, "No trainer team available.");
    if (u->battleRow >= s->foeCount)
      u->battleRow = s->foeCount ? s->foeCount - 1 : 0;
    for (int i = 0; i < s->foeCount; i++) {
      const struct pd_mon *m = &s->foes[i];
      int y = 72 + i * 44;
      if (i == u->battleRow)
        pd_fill(c, 8, y, 496, 42, 0xdcdcdc);
      bool shown = (u->battleSeen & (1u << i)) || i == s->foeActive ||
                   u->battleFoe == i + 1;
      if (!shown) {
        txt(c, 20, y + 10, "Unknown foe Pokemon");
        continue;
      }
      icon(c, 16, y + 3, m->species, rom, n, build);
      snprintf(b, sizeof b, "%s", names[m->species]);
      pd_text_fit(c, 60, y + 2, 240, b, 0x181818, PD_NO_SHADOW);
      snprintf(b, sizeof b, "HP %d/%d %s", m->hp, m->maxHp,
               m->hp ? pd_status_label(m->status) : "FAINTED");
      pd_text_fit(c, 60, y + 22, 244, b, 0x181818, PD_NO_SHADOW);
      pocket_hp_bar(c, 340, y + 2, 154, m->hp, m->maxHp, &polishedHp);
      snprintf(b, sizeof b, "Lv%d%s", m->level,
               i == s->foeActive ? " OUT" : "");
      pd_text_right(c, 494, y + 22, b, 0x181818, PD_NO_SHADOW);
    }
  } else if (!target.species || !player->species)
    txt(c, 20, 94, "Battle data is not ready.");
  else if (u->battlePage == 4) {
    const char *groups[] = {"WEAK TO", "RESISTS", "IMMUNE TO"};
    snprintf(b, sizeof b, "%s Lv%d", names[target.species], target.level);
    txt(c, 12, 66, b);
    txt(c, 12, 96, groups[u->battleType]);
    int ids[18], values[18],
        count = battle_matchups(&target, u->battleType, ids, values);
    int pages = count ? (count + 7) / 8 : 1;
    if (u->battleRow >= pages)
      u->battleRow = pages - 1;
    if (!count)
      txt(c, 20, 136, "None");
    for (int j = 0; j < 8 && u->battleRow * 8 + j < count; j++) {
      int at = u->battleRow * 8 + j, pct = values[at];
      snprintf(b, sizeof b, "%s  %s", typeNames[ids[at]],
               pct == 400   ? "x4"
               : pct == 200 ? "x2"
               : pct == 50  ? "x1/2"
               : pct == 25  ? "x1/4"
                            : "x0");
      txt(c, 20, 130 + 24 * j, b);
    }
  } else if (u->battlePage == 1) {
    snprintf(b, sizeof b, "Against %s Lv%d", names[target.species],
             target.level);
    pd_text_fit(c, 12, 64, 488, b, 0x181818, PD_NO_SHADOW);
    struct battle_pick picks[6];
    int count = battle_rank(s, &target, build, picks);
    if (!count)
      wrapped(c, 20, 112, 472, 3, "No usable damaging moves in your party.");
    for (int i = 0; i < 3 && i < count; i++) {
      struct battle_pick p = picks[i];
      const struct pd_mon *m = &s->party[p.slot];
      int y = 96 + i * 76;
      pd_fill(c, 8, y, 496, 72, 0xececec);
      icon(c, 16, y + 8, m->species, rom, n, build);
      snprintf(b, sizeof b, "%d. %s", i + 1, names[m->species]);
      pd_text_fit(c, 60, y + 4, 240, b, 0x181818, PD_NO_SHADOW);
      pocket_hp_bar(c, 340, y + 4, 154, m->hp, m->maxHp, &polishedHp);
      const char *name;
      int t, power;
      battle_move(m->moves[p.move], build, &name, &t, &power);
      pd_text_fit(c, 60, y + 22, 430, name, 0x181818, PD_NO_SHADOW);
      snprintf(b, sizeof b, "%s  Foe types:%d%%", battle_verdict(power, p.pct),
               p.risk);
      pd_text_fit(c, 60, y + 40, 430, b, 0x181818, PD_NO_SHADOW);
      snprintf(b, sizeof b, "HP %d/%d", m->hp, m->maxHp);
      txt(c, 60, y + 56, b);
      snprintf(b, sizeof b, "Lv%d%s", m->level,
               p.slot == s->partyActive ? " OUT" : "");
      pd_text_right(c, 494, y + 56, b, 0x181818, PD_NO_SHADOW);
    }
    txt(c, 12, 326, "Type/power estimate only.");
  } else if (u->battlePage == 2) {
    snprintf(b, sizeof b, "%s Lv%d HP%d/%d", names[target.species],
             target.level, target.hp, target.maxHp);
    pd_text_fit(c, 12, 66, 488, b, 0x181818, PD_NO_SHADOW);
    snprintf(b, sizeof b, "Held: %s", item_name(target.heldItem, build));
    pd_text_fit(c, 12, 90, 302, b, 0x181818, PD_NO_SHADOW);
    pocket_hp_bar(c, 346, 88, 154, target.hp, target.maxHp, &polishedHp);
    // Stats exist for the active battler only: no fabricated party stats.
    if (!u->battleFoe || u->battleFoe - 1 == s->foeActive) {
      snprintf(b, sizeof b, "Atk%d Def%d Spd%d SpA%d SpD%d", target.stats[0],
               target.stats[1], target.stats[2], target.stats[3],
               target.stats[4]);
      pd_text_fit(c, 12, 112, 488, b, 0x181818, PD_NO_SHADOW);
    }
    battle_moves_draw(c, &target, player, build);
  } else {
    const struct pd_battle_mon *shown[] = {player, &target};
    for (int i = 0; i < 2; i++) {
      int y = 60 + i * 36;
      const struct pd_battle_mon *m = shown[i];
      icon(c, 12, y, m->species, rom, n, build);
      snprintf(b, sizeof b, "%s%s Lv%d", i ? "VS " : "", names[m->species],
               m->level);
      pd_text_fit(c, 58, y, 442, b, 0x181818, PD_NO_SHADOW);
      snprintf(b, sizeof b, "HP %d/%d %s", m->hp, m->maxHp,
               pd_status_label(m->status));
      pd_text_fit(c, 58, y + 18, 250, b, 0x181818, PD_NO_SHADOW);
      pocket_hp_bar(c, 346, y + 18, 154, m->hp, m->maxHp, &polishedHp);
    }
    battle_moves_draw(c, player, &target, build);
  }
  txt(c, 12, 346,
      u->battlePage == 3   ? "D-pad:Choose A:Details"
      : u->battlePage == 4 ? "Left/Right:Group Up/Down:More"
                           : "Select:Live foe Y:Page");
  txt(c, 12, 366,
      u->battlePage || u->battleFoe ? "L/R:Section B:Back X:Play"
                                    : "Start:Settings L/R:Section");
}
