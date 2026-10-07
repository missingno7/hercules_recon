"""Reasoned continuation of the preserved blind effect region; never rewrites its baseline."""
import json
from itertools import product
from env import ROOT
from match import digest
from macro_compare import evaluate
from refinery import run,semantics
from refinery_forms import replace_function,function_text


def particle_forms(source):
    for movement_first,condition in product((True,False),('separate','combined')):
        movement='p->y-=0x10000;'
        if condition=='combined':
            if not movement_first:continue  # Moving y after the callback is not equivalent.
            body=movement+' if(p->ticks^=1) release_effect(p);'
        else:
            updates=[movement,'p->ticks^=1;']
            if not movement_first:updates.reverse()
            body=' '.join(updates)+' if(p->ticks) release_effect(p);'
        yield f'movement{movement_first}-{condition}',replace_function(source,'tick_particle',
            'void tick_particle(Effect *p) { '+body+' }')


def sound_forms(source):
    for zero_before_scale,zero in product((True,False),('separate','chain')):
        stores='p->phase=0; p->ticks=0;' if zero=='separate' else 'p->ticks=p->phase=0;'
        parts=[stores,'p->scale*=3;']
        if not zero_before_scale:parts.reverse()
        body='p->mode=5; p->render_flags|=5; '+' '.join(parts)+'''
            p->y=0x2e00000; p->x=0x4000000; p->z=0x2900000; p->speed=128;
            play_effect_sound(467,0);'''
        yield f'zeros_first{zero_before_scale}-{zero}',replace_function(source,'initialize_sound_effect',
            'void initialize_sound_effect(Effect *p) { '+body+' }')


def start_forms(source):
    body='''if(!g_busy && random_limit(4)<3 && !g_blocked) {
        g_busy=1; select_effect_frame(p,3);
        if(!random_limit(4)) play_effect_sound(450,0);
    }'''
    for label,condition in [('negated','!g_delay'),('explicit','g_delay==0')]:
        yield label,replace_function(source,'maybe_start_effect',
            'void maybe_start_effect(Effect *p) { if('+condition+') { '+body+' } else --g_delay; }')


def main():
    baseline=ROOT/'calibration/refinery_blind.c';spec=ROOT/'evidence/refinery_blind.json'
    config=json.loads(spec.read_text())
    if digest(baseline.read_bytes())!=config['first_source_sha256']:raise ValueError('Baseline changed')
    harness=ROOT/'tests/refinery_blind_semantics.c';include='../calibration/refinery_blind.c'
    root=ROOT/'work/effect_followup';root.mkdir(parents=True,exist_ok=True)
    text=baseline.read_text();results=[];final=text.replace('p->ticks=p->sprite=0;','p->ticks=0; p->sprite=0;')
    for name,family in [('tick_particle',particle_forms),('initialize_sound_effect',sound_forms),('maybe_start_effect',start_forms)]:
        r=run(baseline,spec,'_'+name,harness,include,root/name,family(text))
        results.append(r)
        final=replace_function(final,name,function_text((ROOT/r['best_source']).read_text(),name))
    candidate=root/'effects.c';candidate.write_bytes(final.encode())
    combined=evaluate(candidate,spec,root/'combined.json')
    if combined['summary']['exact']!=4 or combined['summary']['diagnostic_equal']!=12:
        raise ValueError('Recorded effect-region equality did not reproduce')
    semantic=semantics(candidate,harness,{include:candidate},root)
    edge_root=root/'edge_cases';edge_root.mkdir(exist_ok=True)
    edges=semantics(candidate,ROOT/'tests/effect_followup_semantics.c',{include:candidate},edge_root)
    (root/'result.json').write_text(json.dumps(dict(searches=results,combined=combined,semantics=semantic,edge_semantics=edges),indent=2)+'\n')
    print(edges['stdout'])
    return results,combined,semantic


if __name__=='__main__':main()
