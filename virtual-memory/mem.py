import random
import itertools

PHYSICAL_MEM = 512*1024*1024
PAGE_SIZE = 16*1024*1024
VIRTUAL_MEM = 1024*1024*1024
PROCESS_PAGE_COUNT = VIRTUAL_MEM / PAGE_SIZE
PHYSICAL_PAGE_COUNT = PHYSICAL_MEM / PAGE_SIZE

MAX_PROCESS_COUNT = 10
PROCESSES_INCREMENT = 10

PROCESS_ALIVE_MEAN = 500
PROCESS_ALIVE_VARIANCE = 150
TIME = 10000

TIMER_TICK = 50

VALID_BIT = 1
DIRTY_BIT = 2
ACCESS_BIT = 4

class MemoryAllocationError(Exception):
    pass

class NeverShowThisError(Exception):
    pass


class PCB(object):
    def __init__(self, VM, pid):
        self.pid = pid
        self.pages = {}  # key: local index, value: global index
        # self.pages = []
        self.last_page = None
        self.VM = VM
        self.renew()

    def renew(self):
        for p in self.pages.values():
            self.VM.kill_page(p)
        self.pages = {}
        self.alive = abs(random.normalvariate(PROCESS_ALIVE_MEAN, PROCESS_ALIVE_VARIANCE))
        self.last_page = None

    def quant(self):
        self.alive -= 1
        if self.alive < 0:
            print "Stopping {%d} process" % self.pid
            self.renew()
            return

        vpid = None
        if random.random() < 0.9 and self.last_page:
            vpid = self.last_page
        else:
            vplid = random.randint(1, PROCESS_PAGE_COUNT)
            if vplid not in self.pages:
                self.pages[vplid] = self.VM.request_page(self.pid)

            vpid = self.pages[vplid]

        self.VM.access_page(vpid, write=random.random() > 0.5)
        self.last_page = vpid


class VPage(dict):
    pass


class VM(object):
    def __init__(self, algorithm):
        self.processes = []
        self.empty_pages = []
        self.virtual_pages = {}  # index:VPage objects
        self.vpid = 0
        self.pid_counter = itertools.count()
        self.choice_algorithm = algorithm
        self.pf = 0
        self.accesses = 0

    def start(self):
        print "Starting model:"
        print "  Physical RAM / pages: %d / %d" % (PHYSICAL_MEM, PHYSICAL_PAGE_COUNT)
        print "  Virtual RAM per process / pages: %d / %d" % (VIRTUAL_MEM, PROCESS_PAGE_COUNT)
        print "="*80
        self.pf = 0
        self.accesses = 0

        self.empty_pages = [i for i in range(PHYSICAL_PAGE_COUNT)]

        timer = itertools.count(TIMER_TICK, -1)
        while len(self.processes) < MAX_PROCESS_COUNT+1:
            self.processes += [PCB(self, next(self.pid_counter), ) for i in range(PROCESSES_INCREMENT)]
            for _ in xrange(TIME/len(self.processes)):
                for p in self.processes:
                    p.quant()
                    self.reset_bits()
                # if next(timer) < 0:
                    # self.reset_bits()
                    # timer = itertools.count(TIMER_TICK, -1)

        print "="*80

        RES = len([vpage for vpage in self.virtual_pages.values() if vpage['flags'] & VALID_BIT])
        VIRT = len([vpage for vpage in self.virtual_pages.values()])
        print "RES: %d (%0.2f%%)" % (RES*PAGE_SIZE, (100.0*RES)/PHYSICAL_PAGE_COUNT)
        print "VIRT/pages: %d / %d (%0.2f%%)" % (VIRT*PAGE_SIZE, VIRT, (100.0*VIRT)/(MAX_PROCESS_COUNT*PROCESS_PAGE_COUNT))
        print "pf rate: %d (%0.2f%%)" % (self.pf, 100*self.pf/float(self.accesses))

    def reset_bits(self):
        for i in [vpage['id'] for vpage in self.virtual_pages.values() if vpage['flags'] & VALID_BIT]:
            page = self.virtual_pages[i]
            page['countdown'] -= 1
            if page['countdown'] < 0:
                print "reset flags for [%d]" % (i)
                page['flags'] &= ~ACCESS_BIT
                page['flags'] &= ~DIRTY_BIT
                page['countdown'] = TIMER_TICK


    def request_page(self, pid):
        print "{%d} requested new page" % pid
        vpid = self.vpid
        self.vpid += 1
        self.virtual_pages[vpid] = VPage(id=vpid, ppid=None, flags=0, countdown=TIMER_TICK)
        self.make_resident(vpid, nofault=True)
        return vpid

    def make_resident(self, vpid, nofault=False):
        page = self.virtual_pages[vpid]
        if not (page['flags'] & VALID_BIT):
            if not nofault:
                print "Page fault accessing [%d]" % (vpid)
                self.pf += 1
            ppid = self.need_empty_physical_page()

            print " . moving [%d] to physical ram as %d" % (vpid, ppid)
            if ppid is None:
                raise MemoryAllocationError()
            page['ppid'] = ppid
            page['flags'] |= VALID_BIT
            page['flags'] &= ~ACCESS_BIT
            page['flags'] &= ~DIRTY_BIT
            page['countdown'] = TIMER_TICK

    def access_page(self, vpid, write):
        print ">> accessing [%d]" % vpid
        self.accesses += 1
        self.make_resident(vpid)
        page = self.virtual_pages[vpid]
        if write:
            page['flags'] |= DIRTY_BIT
        page['flags'] |= ACCESS_BIT

    def need_empty_physical_page(self):
        if not self.empty_pages:
            self.replace_page()
        return self.empty_pages.pop()

    def replace_page(self):
        vpid = self.choice_algorithm(self.virtual_pages)
        page = self.virtual_pages[vpid]
        ppid = page['ppid']
        page['flags'] &= ~VALID_BIT
        page['flags'] &= ~ACCESS_BIT
        page['flags'] &= ~DIRTY_BIT
        self.empty_pages.append(ppid)
        print " . swapping [%d] page. Now %d free" % (vpid, ppid)

    def kill_page(self, vpid):
        page = self.virtual_pages[vpid]
        ppid = page['ppid']
        del self.virtual_pages[vpid]
        if page['flags'] & VALID_BIT:
            self.empty_pages.append(ppid)
        print " . deleting [%d] page. Now %d free" % (vpid, ppid)


def choice_random(virtual_pages):
    vpid = random.choice([vpage for vpage in virtual_pages.values() if vpage['flags'] & VALID_BIT])['id']
    return vpid

def choice_nru(virtual_pages):
    lst = sorted([vpage for vpage in virtual_pages.values() if (vpage['flags'] & VALID_BIT)], key=lambda x: x['flags'], reverse=False)
    category = lst[0]['flags']
    pick_list = filter(lambda p: p['flags'] == category, lst)
    vpid = random.choice(pick_list)['id']
    return vpid

def choice_lru(virtual_pages):
    lst = sorted([vpage for vpage in virtual_pages.values() if (vpage['flags'] & VALID_BIT)], key=lambda x: x['flags'], reverse=True)
    category = lst[0]['flags']
    pick_list = filter(lambda p: p['flags'] == category, lst)
    vpid = random.choice(pick_list)['id']
    return vpid


if __name__ == '__main__':
    model = VM(algorithm=choice_nru)
    model.start()

