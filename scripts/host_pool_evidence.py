"""Recheck reviewed host pool and factory spans without loading original images."""
import json

from env import ROOT
from match import digest
from pe import PE


def verify():
    total = 0
    names = ('host_pool_layout', 'actor_factory_layout', 'actor_pool_layout', 'host_pool_control')
    for name in names:
        evidence = json.loads((ROOT / ('evidence/' + name + '.json')).read_text())
        oracle = evidence['oracle']
        path = ROOT / oracle['path']
        if digest(path.read_bytes()) != oracle['sha256']:
            raise ValueError('Changed oracle: ' + name)
        image = PE(path)
        for span in evidence['code_spans']:
            start = int(span['start_rva'], 0)
            end = int(span['end_rva_exclusive'], 0)
            if digest(image.read_rva(start, end - start)) != span['sha256']:
                raise ValueError('Changed reviewed span: ' + name + ' ' + span['start_rva'])
            total += 1
        if name == 'host_pool_layout':
            table = evidence['source_table']
            if digest(image.read_rva(int(table['rva'], 0), table['bytes'])) != table['sha256']:
                raise ValueError('Changed default host interface table')
    counts = json.loads((ROOT / 'evidence/host_pool_counts_layout.json').read_text())
    path = ROOT / counts['oracle']['path']
    if digest(path.read_bytes()) != counts['oracle']['sha256']:
        raise ValueError('Changed count-helper oracle')
    span = counts['function_extent']
    if digest(PE(path).read_rva(int(span['rva'], 0), span['bytes'])) != span['sha256']:
        raise ValueError('Changed count-helper span')
    result = dict(scope='Reviewed static bytes only; no original declarations or runtime outcome proof.',
                  images=2, evidence_sets=len(names)+1, code_spans=total+1)
    print(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    verify()
