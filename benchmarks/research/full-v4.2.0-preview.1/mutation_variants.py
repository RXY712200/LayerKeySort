from pathlib import Path
p=Path('benchmarks/research/full-v4.2.0-preview.1')
def mutation(s):
 old='    } else v4_locations(order, block);'
 assert s.count(old)==1
 s=s.replace(old,'''    } else {
        record->block = block;
        V4_COUNT(order, local_blocks);
        for (i = at; i < block->count; ++i) {
            block->records[i]->local = i;
            V4_COUNT(order, records_reassigned);
        }
    }''')
 old='    else v4_locations(order, block);'
 assert s.count(old)==1
 s=s.replace(old,'''    else {
        V4_COUNT(order, local_blocks);
        block->records[block->count] = NULL;
        for (i = record->local; i < block->count; ++i) {
            block->records[i]->local = i;
            V4_COUNT(order, records_reassigned);
        }
    }''')
 return s.replace('        else v4_locations(order, source);','''        else {
            V4_COUNT(order, local_blocks);
            for (i = from; i < source->count; ++i) {
                source->records[i]->local = i;
                V4_COUNT(order, records_reassigned);
            }
        }''')
for name,src in [('mutation',Path('src/order.c')),('combined',p/'ancestor-order.c.txt')]:
 (p/(name+'-order.c.txt')).write_text(mutation(src.read_text()),encoding='utf-8',newline='\n')
