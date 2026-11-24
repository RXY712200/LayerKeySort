"""Stage exact control or a research variant in a disposable source directory."""
import argparse
import io
from pathlib import Path
import subprocess
import tarfile

PIN = '22544cf283694dd27cef203caba9cf1838845947'
HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('target', type=Path)
parser.add_argument('variant', choices=['control','tags','no-count'])
args = parser.parse_args()
target = args.target.resolve()
target.mkdir()
data = subprocess.check_output(['git', 'archive', PIN])
with tarfile.open(fileobj=io.BytesIO(data)) as archive:
    archive.extractall(target, filter='data')

def edit(name, before, after):
    path = target/'src'/name
    text = path.read_text(encoding='utf-8')
    assert text.count(before)==1, before
    path.write_text(text.replace(before, after), encoding='utf-8', newline='\n')

if args.variant == 'tags':
    edit('lks_order_internal.h', 'size_t subtree_blocks, count;',
         'size_t subtree_blocks, count;\n    uint64_t tag;')
    edit('lks_order_internal.h', '    LksOrderWork work;',
         '    LksOrderWork work;\n    size_t tag_writes, tag_repairs, tag_fallbacks, max_tag_writes;')
    edit('order.c', '/* Called only with a prepared block;',
         (HERE/'tag_maintenance.inc').read_text()+'\n/* Called only with a prepared block;')
    edit('order.c', '    ++order->blocks;\n    v4_balance(order, parent);',
         '    ++order->blocks;\n    preview_tag_attach(order, block);\n    v4_balance(order, parent);')
    edit('order.c', '    /* Height strictly increases',
         '    if (left->tag && right->tag) {\n        *out = left->tag < right->tag ? -1 : 1;\n        return LKS_STATUS_OK;\n    }\n    /* Height strictly increases')
    edit('order_bulk.c', 'b->owner = o; b->previous',
         'b->owner = o; b->tag = ((uint64_t)i+1)*(UINT64_MAX/((uint64_t)blocks+1)); b->previous')
elif args.variant == 'no-count':
    edit('lks_order_internal.h', 'size_t subtree_blocks, count;', 'size_t count;')
    edit('order.c', 'static size_t v4_subtree(const LksOrderBlock *block)\n{ return block ? block->subtree_blocks : 0; }\n', '')
    edit('order.c', '    block->subtree_blocks = 1 + v4_subtree(block->left) + v4_subtree(block->right);\n', '')
    edit('order.c', ' block->subtree_blocks = 1;', '')
    edit('order.c', ' && block->subtree_blocks == *out_blocks', '')
    edit('order_bulk.c', ' root->subtree_blocks = count;', '')
