"""Strict adapter for upstream's Polished guide, not a new game-data source."""
import json, re
INPUTS = ['GuidePolishedCrystal.kt', 'GuidePolishedCrystalGen.kt']
def generate(base):
    src = (base / INPUTS[1]).read_text()
    ui = (base / INPUTS[0]).read_text()
    constants = dict((k, int(v, 0)) for k,v in re.findall(r'internal const val (PC_\w+) = (0x[0-9A-Fa-f]+|\d+)', src))
    out = ['/* Generated from GuidePolishedCrystal*.kt. GPL-3.0. */',
           'enum {' + ','.join(k+'='+str(v) for k,v in constants.items()) + '};']
    levels = re.search(r'PC_BADGE_BASE_LEVELS = intArrayOf\(([^)]+)\)', src).group(1)
    assert len(levels.split(',')) == 17
    out += ['static const int guideBadgeLevels[] = {'+levels+'};']
    def lines(name):
        section = src.split('val '+name+': List<String> = listOf(',1)[1].split('\n)',1)[0]
        chunks = re.findall(r'"""(.*?)"""', section, re.S)
        assert chunks
        return [s for chunk in chunks for s in chunk.strip().splitlines() if s]
    wild = [s.split('|') for s in lines('PC_WILD_RAW')]
    fish = {int(r[1]):r[2:] for r in wild if r[0]=='FG'}
    trees = {int(r[1]):r[2:] for r in wild if r[0]=='TS'}
    rows=[]
    for f in wild:
        kind=f[0]
        if kind in ('FG','TS'): continue
        key=int(f[1])*256+int(f[2])
        def row(method,tod,level,species,weight):
            assert 0<=species<512 and 0<=level<=255 and weight>0
            rows.append('{'+','.join(map(str,[key,method,tod,level,species,weight]))+'}')
        if kind=='G':
            slots=[list(map(int,p.split(':'))) for p in f[4].split()]
            rates=list(map(int,f[3].split(',')))
            assert len(slots)==21 and len(rates)==3
            for t in range(3):
                if rates[t]:
                    for (level,species),w in zip(slots[7*t:7*t+7],[30,30,20,10,5,3,2]):row(0,t,level,species,w)
        elif kind=='W':
            slots=[list(map(int,p.split(':'))) for p in f[4].split()]
            assert len(slots)==3
            if int(f[3]):
                for (level,species),w in zip(slots,[60,30,10]):row(1,-1,level,species,w)
        elif kind=='F':
            assert len(fish[int(f[3])])==3
            for method,slots in enumerate(fish[int(f[3])],2):
                for p in slots.split():
                    weight,species,level=map(int,p.split(':'));row(method,-1,level,species,weight)
        elif kind in ('T','R'):
            for method,slots in enumerate(trees[int(f[3])]):
                if kind=='R' and method:continue
                for p in slots.split():
                    weight,species,level=map(int,p.split(':'));row(7 if kind=='R' else 5+method,-1,level,species,weight)
        else:raise ValueError('Unknown wild row '+str(f))
    assert len(rows)>4000
    out += ['static const struct {unsigned short map; signed char method,tod; unsigned char level; unsigned short species,weight;} guideWild[] = {'+','.join(rows)+'};']
    teams={}
    for faithful,name in [(0,'PC_BOSSES_RAW'),(1,'PC_BOSSES_FAITHFUL_RAW')]:
        for s in lines(name):
            f=s.split('|'); mons=[]
            for p in f[4].split(';'):
                v=p.split();assert len(v)==4
                lv,species,item=map(int,v[:3]);moves=list(map(int,v[3].split(',')))
                assert len(moves)==4
                mons.append('{'+','.join(map(str,[lv,species,item]))+',{'+','.join(map(str,moves))+'}}')
            assert 1<=len(mons)<=6
            teams[(faithful,f[0])]=(len(mons),'{'+','.join(mons)+'}')
    bosses=[]
    boss_section=ui.split('private val BOSSES_POLISHED_CRYSTAL = listOf(',1)[1].split('\n)',1)[0]
    for line in boss_section.splitlines():
        if not line.strip():continue
        m=re.fullmatch(r'\s*pcBoss\("([^"]+)", "([^"]+)", (\w+), "([^"]+)"(?:, pc(League|Rematch)\((\w+)\))?\),',line)
        assert m, 'Upstream boss grammar changed: '+line
        title,where,event,key,rule,own=m.groups()
        done=constants[event] if event!='0' else constants[own]
        mode={'League':1,'Rematch':2,None:0}[rule]
        normal=teams[(0,key)];faithful=teams.get((1,key),normal)
        assert normal[0]==faithful[0]
        bosses.append('{'+','.join([json.dumps(title),json.dumps(where),str(done),str(mode),str(normal[0]),normal[1],faithful[1]])+'}')
    assert len(bosses)==27
    out += ['struct guide_team_mon {int level,species,item,moves[4];};',
            'static const struct {const char *title,*where; int flag,rule,count; struct guide_team_mon normal[6],faithful[6];} guideBosses[] = {'+','.join(bosses)+'};']
    kinds=['ITEM','HIDDEN','GIFT','KEY','MON','EGG','TRADE']
    areas=[]
    for line in lines('GUIDE_AREAS_POLISHED_CRYSTAL_RAW'):
        f=line.split('|');assert len(f)==7 and f[1] in kinds
        area,kind,id,flag,qty,wants,where=f
        areas.append('{'+','.join([area,str(kinds.index(kind)),id,flag,qty,wants,json.dumps(where)])+'}')
    assert len(areas)>400
    out += ['static const struct {int area,kind,id,flag,qty,wants; const char *where;} guideAreas[] = {'+','.join(areas)+'};']
    # The seven upstream TIPS entries. Join Kotlin string concatenation, preserving words.
    tips=[]
    for block in re.findall(r'\bentry\(\n(.*?)\n\s*\),',ui,re.S):
        def strings(s):return ''.join(json.loads(x) for x in re.findall(r'"(?:[^"\\]|\\.)*"',s))
        title,rest=block.split(',\n',1)
        answer,*opts=rest.split(',\n')
        hint=''
        for opt in opts:
            if 'hint =' in opt:hint=strings(opt)
        tips.append('{'+','.join(json.dumps(x) for x in [strings(title),hint,strings(answer)])+'}')
    assert len(tips)==7
    out += ['static const struct {const char *title,*hint,*answer;} guideTips[] = {'+','.join(tips)+'};']
    return '\n'.join(out)+'\n'
