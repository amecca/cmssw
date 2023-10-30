#!/usr/bin/env python3

from argparse import ArgumentParser
import logging
import ROOT

class TFileContext(object):
    def __init__(self, *args):
        self.tfile = ROOT.TFile(*args)
    def __enter__(self):
        return self.tfile
    def __exit__(self, type, value, traceback):
        self.tfile.Close()

def display_results(t_data, style='twiki'):
    if  (style == 'twiki'):
        header_fmt = ' | '.join( ['{:^6s}'] + ['{:^9s}' ] * (len(t_data.keys())-1) )
        row_fmt    = ' | '.join( ['{:6d}' ] + ['{:9.6f}'] * (len(t_data.keys())-1) )
        header_fmt = '| '+header_fmt+' |'
        row_fmt    = '| '+row_fmt   +' |'
    elif(style == 'latex'):
        header_fmt = ' & '.join( ['{:^6s}'] + ['{:^9s}' ] * (len(t_data.keys())-1) )  + ' \\\\\n\\hline'
        row_fmt    = ' & '.join( ['{:6d}' ] + ['{:9.6f}'] * (len(t_data.keys())-1) ) + ' \\\\'
    elif(style == 'csv'):
        header_fmt = ', '.join(  ['{:s}'  ]                *  len(t_data.keys())    )
        row_fmt    = ', '.join(  ['{:d}'  ] + ['{:f}'    ] * (len(t_data.keys())-1) )
    else:
        raise RuntimeError('Unknown style "%s" for table'%(style))

    print( header_fmt.format(*t_data.keys()) )
    for i, run in enumerate(t_data['run']):
        print(row_fmt.format(run, *[t_data[k][i] for k in t_data.keys() if not k == 'run']))


def main():
    parser = ArgumentParser()
    parser.add_argument('fname'            , metavar='FILE')
    parser.add_argument('-p', '--partition', default='BPIX', help='Tracker partition (e.g. BPIX, FPIX, BPIXLYR1, etc.). Default: %(default)s')
    parser.add_argument('-l', '--list'         , action='store_true', dest='list_entries', help='List the contents of file and exit')
    parser.add_argument(      '--list-branches', action='store_true', help='List the branches of the tree and exit')
    parser.add_argument('-t', '--type'     , default='barycentre', choices=('barycentre', 'beamspot'), type=str.lower, help='Default: %(default)s')
    parser.add_argument(      '--label'    , default=None, help='Additional label that is appended to the folder name')
    parser.add_argument(      '--quality'  , action='store_true', help='Read results with the WithPixelQuality flag (default: %(default)s)')
    parser.add_argument('-s', '--style'    , default='twiki', choices=('twiki', 'latex', 'csv'), type=str.lower, help='Table style for the output (choices: %(choices)s)')
    parser.add_argument(      '--loglevel' , metavar='LEVEL', default='WARNING', help='Level for the python logging module. Can be either a mnemonic string like DEBUG, INFO or WARNING or an integer (lower means more verbose).')

    args = parser.parse_args()
    loglevel = args.loglevel.upper() if not args.loglevel.isdigit() else int(args.loglevel)
    logging.basicConfig(format='%(levelname)s:%(module)s:%(funcName)s: %(message)s', level=loglevel)
    logging.debug('args: %s', args)

    if(args.list_entries):
        list_entries(args.fname)
        return 0

    folder    = 'PixelBaryCentreAnalyzer' if not args.quality          else 'PixelBaryCentreAnalyzerWithPixelQuality'
    tree_name = 'PixelBarycentre'         if args.type == 'barycentre' else 'BeamSpot'
    if(args.label is not None):
        tree_name += '_'+args.label
    columns = ['run'] + [(args.partition if args.type=='barycentre' else 'BS')+'.'+coord for coord in ('x', 'y', 'z')]

    with TFileContext(args.fname) as tfile:
        tfolder = tfile.Get(folder)
        if(not tfolder):
            raise KeyError('Folder "%s" not found in "%s"' %(folder, args.fname))

        logging.debug('Opened folder "%s"', folder)
        tree = tfolder.Get(tree_name)
        if(not tree):
            logging.error('Tree "%s" not found; content of file "%s":', tree_name, args.fname)
            list_entries(args.fname)
            raise KeyError(tree_name)

        if(args.list_branches):
            print('Branches (%s/%s): %s' % (folder, tree_name, [b.GetName() for b in tree.GetListOfBranches()]))
            return 0
        rdf = ROOT.RDataFrame(tree)
        logging.info('Reading "%s"', tree_name)
        results = rdf.AsNumpy(columns)

    display_results(results, style=args.style)

    return 0

def list_entries(fname):
    with TFileContext(fname) as tfile:
        for tfolder in tfile.GetListOfKeys():
            print(tfolder.GetName())
            for key in tfolder.ReadObj().GetListOfKeys():
                tree = key.ReadObj()
                print('\t%s %-16s: %d branches, %d entries' %(tree.ClassName(), key.GetName(), len(tree.GetListOfBranches()), tree.GetEntries()))

if __name__ == '__main__':
    exit(main())
