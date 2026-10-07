"""Replay bounded calibration families; generated candidates and logs stay ignored."""
from env import ROOT
from refinery import run
from refinery_forms import table_pointer_forms,guarded_tag_forms


def main():
    source=ROOT/'calibration/macro_actor.c';spec=ROOT/'evidence/macro_actor.json'
    harness=ROOT/'tests/macro_semantics.c';include='../calibration/macro_actor.c'
    output=ROOT/'work/refinery_replay'
    for symbol,name in [('_change_kind','kind'),('_create_command','command'),('_find_tag','tag')]:
        run(source,spec,symbol,harness,include,output/name)
    run(source,spec,'_change_kind',harness,include,output/'kind_tables',table_pointer_forms(source.read_text()))
    run(source,spec,'_find_tag',harness,include,output/'tag_guarded',guarded_tag_forms(source.read_text()))
    run(ROOT/'src/shared/relative.c',ROOT/'evidence/refinery_relative.json','_measure_relative_vector',
        ROOT/'tests/shared_semantics.c','../src/shared/relative.c',output/'relative')


if __name__=='__main__':main()
