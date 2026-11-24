from pathlib import Path
import subprocess
p=Path(__file__).resolve().parent
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
stable = subprocess.check_output(['git','show','70d7f0ef5857b54381b863d292a213909d87459f:src/order.c'],text=True)
for name,src in [('mutation',stable),('combined',(p/'ancestor-order.c.txt').read_text())]:
 with (p/(name+'-reproduced.c.txt')).open('x',encoding='utf-8',newline='\n') as output:
  output.write(mutation(src))
