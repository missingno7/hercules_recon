"""Bounded natural C families. No flags, layout, padding, or dummy declarations vary."""
from itertools import product
import re


def replace_function(source, name, replacement):
    start = re.search(r'^\w[^\n]*\b' + re.escape(name) + r'\([^;]*?\)\s*\{', source, re.M)
    if not start:
        raise ValueError(name)
    opening = source.index('{', start.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[:start.start()] + replacement + source[end:]


def function_text(source, name):
    marker = '/* REFINERY FUNCTION */'
    stripped = replace_function(source, name, marker)
    before, after = stripped.split(marker)
    return source[len(before):len(source)-len(after)]


def kind_forms():
    # Re-evaluate table globals at each original read, preserving alias ordering.
    for address, index_type in product(('indexed', 'base_offset', 'offset_base', 'pointer'),
                                       ('inline', 'unsigned int', 'unsigned short', 'after_store')):
        branches = []
        for prefix, asset, callback, index in (
            ('special', 'Asset8', 'Callback8', 'kind & 0xfff'),
            ('alternate', 'Asset8', 'Callback8', 'kind & 0xfff'),
            ('normal', 'Asset16', 'Callback12', 'kind')):
            lines = ['p->kind = kind;']
            idx = '('+index+')' if index_type == 'inline' else 'index'
            if index_type == 'after_store': lines = ['unsigned int index;', 'p->kind = kind;', f'index = {index};']
            elif index_type != 'inline': lines.insert(0,f'{index_type} index = {index};')
            def access(table, typ, field):
                base = f'g_{prefix}_{table}'
                if address == 'indexed': return f'{base}[{idx}].{field}'
                if address == 'pointer': return f'({base} + {idx})->{field}'
                offset = f'{idx} * sizeof({typ})'
                expr = f'(char *){base} + {offset}' if address == 'base_offset' else f'{offset} + (char *){base}'
                return f'(({typ} *)({expr}))->{field}'
            lines += ['p->category = '+access('assets',asset,'category')+';',
                      'p->image = '+access('assets',asset,'image')+';',
                      access('callbacks',callback,'init')+'(p);']
            branches.append('\n'.join(lines))
        yield f'{address}-{index_type}', '''void change_kind(Actor *p, unsigned short kind)
{ if (p->kind & 0x6000) { if (p->kind & 0x4000) {
%s
} else {
%s
} } else {
%s
} reset_frame_state(p); refresh_actor(p); }''' % tuple(branches)


def tag_forms():
    for cursor, loop, increment in product(('pre', 'post', 'index'), ('for', 'while'), ('before', 'after')):
        init = 'g_tag_list' if cursor == 'pre' else 'g_tag_list + 1'
        read = {'pre':'*++entry','post':'*entry++','index':'g_tag_list[i + 1]'}[cursor]
        body = 'p = (Actor *)'+read+';\n'
        if increment == 'before': body += '++i;\n'
        body += 'if (p->tag == tag) return p;\n'
        if increment == 'after': body += '++i;\n'
        header = 'for (i = 0; i < count; )' if loop == 'for' else 'i = 0; while (i < count)'
        decl = '' if cursor == 'index' else f'unsigned long *entry = {init};'
        yield f'{cursor}-{loop}-{increment}', f'''Actor *find_tag(int tag)
{{ int i, count = g_tag_list[0]; {decl} Actor *p;
{header} {{ {body} }} return 0; }}'''


def guarded_tag_forms(source):
    # A second, explicitly accounted model intervention: keep cursor setup
    # inside the positive-count lifetime, then mechanically vary loop spelling.
    for guard,loop in product(('positive','early-return'),('for','do')):
        body='p = (Actor *)*++entry; ++i; if (p->tag == tag) return p;'
        traversal=f'for (i=0; i<count; ) {{ {body} }}' if loop=='for' else f'i=0; do {{ {body} }} while(i<count);'
        inner=f'entry=g_tag_list; {traversal}'
        code=f'if(count>0) {{ {inner} }}' if guard=='positive' else f'if(count<=0) return 0; {inner}'
        replacement=f'Actor *find_tag(int tag) {{ int i,count=g_tag_list[0]; unsigned long *entry; Actor *p; {code} return 0; }}'
        yield f'{guard}-{loop}',replace_function(source,'find_tag',replacement)


def table_pointer_forms(source):
    for order,access in product(('table-first','index-first'),('array','pointer','byte-offset')):
        branches=[]
        for prefix,asset,callback,index in [('special','Asset8','Callback8','kind & 0xfff'),('alternate','Asset8','Callback8','kind & 0xfff'),('normal','Asset16','Callback12','kind')]:
            initial=[f'assets=g_{prefix}_assets;',f'index={index};']
            if order=='index-first':initial.reverse()
            def member(base,typ,field):
                if access=='array':return f'{base}[index].{field}'
                if access=='pointer':return f'({base}+index)->{field}'
                return f'(({typ} *)((char *){base}+index*sizeof({typ})))->{field}'
            branches.append(f'''{asset} *assets; {callback} *callbacks; unsigned int index;
                p->kind=kind; {' '.join(initial)}
                p->category={member('assets',asset,'category')};
                assets=g_{prefix}_assets;
                p->image={member('assets',asset,'image')};
                callbacks=g_{prefix}_callbacks;
                {member('callbacks',callback,'init')}(p);''')
        code='''void change_kind(Actor *p,unsigned short kind) {
            if(p->kind & 0x6000) { if(p->kind & 0x4000) {%s} else {%s} }
            else {%s} reset_frame_state(p);refresh_actor(p); }''' % tuple(branches)
        yield f'{order}-{access}',replace_function(source,'change_kind',code)


def command_forms():
    for cached_flags, result, grouping in product((False, True), ('direct', 'early', 'late'), ('chains','individual')):
        decl = 'unsigned long flags;' if cached_flags else ''
        decl += ' int result = 0;' if result != 'direct' else ''
        body = 'command = *handle;\n'
        if cached_flags: body += 'flags = p->flags;\n'
        body += 'p->command = handle;\n'
        if result == 'early': body += 'result = 1;\n'
        body += 'p->flags = flags | 0x20000000;\n' if cached_flags else 'p->flags |= 0x20000000;\n'
        body += 'command[0] |= 0x8000;\n'
        if grouping == 'chains':
            body += 'command[1] = command[2] = command[3] = 0;\n'
            body += '\n'.join(f'command[{i}] = command[{i+1}] = 0;' for i in (4,6,8))
        else:
            body += '\n'.join(f'command[{i}] = 0;' for i in (3,2,1,5,4,7,6,9,8))
        body += '\nreturn 1;' if result == 'direct' else '\n'+('result = 1;' if result == 'late' else '')
        yield f'flags{cached_flags}-{result}-{grouping}', f'''int create_command(Actor *p)
{{ unsigned short **handle = allocate_command(0x3781); unsigned short *command;
{decl} if (handle) {{ {body} }} return {('0' if result == 'direct' else 'result')}; }}'''


def relative_forms(source):
    original = function_text(source, 'measure_relative_vector')
    old = 'out->planar_distance = x + (y >> 3) + (y >> 2);'
    for order, grouping, narrowing in product((('3','2'),('2','3')), ('left','right','temps'), ('int','short')):
        a,b = order
        if grouping == 'temps':
            expr = f'{{ {narrowing} first = y >> {a}; {narrowing} second = y >> {b}; out->planar_distance = x + first + second; }}'
        else:
            term = lambda n: f'({narrowing})(y >> {n})'
            expr = f'x + {term(a)} + {term(b)}' if grouping == 'left' else f'x + ({term(a)} + {term(b)})'
            expr = 'out->planar_distance = '+expr+';'
        yield f'{a}{b}-{grouping}-{narrowing}', original.replace(old,expr)


def variants(source, name):
    families = {'change_kind':kind_forms, 'create_command':command_forms, 'find_tag':tag_forms}
    forms = relative_forms(source) if name == 'measure_relative_vector' else families[name]()
    seen = {source}
    for label, function in forms:
        candidate = replace_function(source,name,function)
        if candidate not in seen:
            seen.add(candidate)
            yield label,candidate


def zero_store_forms(source,name,old,fields):
    """Independent, nonvolatile fields: separate stores and either assignment chain."""
    original=function_text(source,name)
    a,b=fields
    for label,text in [('separate',f'{a}=0; {b}=0;'),
                       ('chain-reversed',f'{b}={a}=0;'),
                       ('separate-reversed',f'{b}=0; {a}=0;')]:
        yield label,replace_function(source,name,original.replace(old,text))
