"""Replay the immutable blind first pass, then its single bounded near-codegen family."""
import json
from env import ROOT
from match import digest
from macro_compare import evaluate
from refinery import run,semantics
from refinery_forms import zero_store_forms


def main():
    source=ROOT/'calibration/refinery_blind.c'
    spec=ROOT/'evidence/refinery_blind.json'
    config=json.loads(spec.read_text())
    if digest(source.read_bytes())!=config['first_source_sha256']:
        raise ValueError('Blind first-pass source changed')
    output=ROOT/'work/refinery/blind';output.mkdir(parents=True,exist_ok=True)
    baseline=evaluate(source,spec,output/'baseline.json')
    semantic=semantics(source,ROOT/'tests/refinery_blind_semantics.c',{'../calibration/refinery_blind.c':source},output)
    near=[r['symbol'] for r in baseline['functions'] if r['category']=='near codegen candidate']
    if near!=['_initialize_burst']:
        raise ValueError('Blind classification changed; review instead of selecting targets silently')
    result=run(source,spec,near[0],ROOT/'tests/refinery_blind_semantics.c','../calibration/refinery_blind.c',output/'burst',
               zero_store_forms(source.read_text(),'initialize_burst','p->ticks=p->sprite=0;',('p->ticks','p->sprite')))
    return baseline,semantic,result


if __name__=='__main__': main()
