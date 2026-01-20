BASEDIR=.
SRCDIR=$(BASEDIR)/src
# PRSRCDIR=$(BASEDIR)/src/pr/w_ofis
HDRDIR=$(BASEDIR)/header
OBJDIR=$(BASEDIR)/obj

CPP=gcc
NUMTASKLETS=-DNR_TASKLETS=16
HOSTFLAGS=-O3 -fopenmp -w -lpthread
# USEDPUS=-ldpu -I/usr/include/dpu
USEDPUS=`dpu-pkg-config --cflags --libs dpu` $(NUMTASKLETS)
CLANG=dpu-upmem-dpurte-clang
PIMFLAGS=$(NUMTASKLETS) -O3 
HEADER=$(HDRDIR)/host.h $(HDRDIR)/common.h

all: pr_mram pr_wram pr_ofis

# PageRank - MRAM
PRSRCDIR=$(BASEDIR)/src/pr/origin_sdk
pr_mram: pr_mram_main pr_mram_kernel

pr_mram_main: $(OBJDIR)/pr_mram_main.o $(OBJDIR)/readInput.o $(OBJDIR)/preProc.o $(OBJDIR)/pr_mram_host.o $(HEADER)
	$(CPP) $(HOSTFLAGS) -o bin/pr_mram $^ $(USEDPUS)

$(OBJDIR)/pr_mram_main.o: $(PRSRCDIR)/main.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)

$(OBJDIR)/pr_mram_host.o: $(PRSRCDIR)/pr_host.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)

pr_mram_kernel: $(PRSRCDIR)/pr_kernel.c $(HEADER)
	$(CLANG) $(PIMFLAGS) -o $(BASEDIR)/bin/pr_mram_kernel $<
	

# PageRank - WRAM
PRSRCDIR=$(BASEDIR)/src/pr/wram_sdk
pr_wram: pr_wram_main pr_wram_kernel

pr_wram_main: $(OBJDIR)/pr_wram_main.o $(OBJDIR)/readInput.o $(OBJDIR)/preProc.o $(OBJDIR)/pr_wram_host.o $(HEADER)
	$(CPP) $(HOSTFLAGS) -o bin/pr_wram $^ $(USEDPUS)

$(OBJDIR)/pr_wram_main.o: $(PRSRCDIR)/main.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)

$(OBJDIR)/pr_wram_host.o: $(PRSRCDIR)/pr_host.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)

pr_wram_kernel: $(PRSRCDIR)/pr_kernel.c $(HEADER)
	$(CLANG) $(PIMFLAGS) -o $(BASEDIR)/bin/pr_wram_kernel $<  

# PageRank - OFIS
PRSRCDIR=$(BASEDIR)/src/pr/w_ofis
pr_ofis: pr_ofis_main pr_ofis_kernel

pr_ofis_main: $(OBJDIR)/pr_ofis_main.o $(OBJDIR)/readInput.o $(OBJDIR)/preProc.o $(OBJDIR)/pr_ofis_host.o $(HEADER)
	$(CPP) $(HOSTFLAGS) -o bin/pr_ofis $^ $(USEDPUS)

$(OBJDIR)/pr_ofis_main.o: $(PRSRCDIR)/main.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)

$(OBJDIR)/pr_ofis_host.o: $(PRSRCDIR)/pr_host.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)

pr_ofis_kernel: $(PRSRCDIR)/pr_kernel.c $(HEADER)
	$(CLANG) $(PIMFLAGS) -o $(BASEDIR)/bin/pr_ofis_kernel $<  

# Common
$(OBJDIR)/readInput.o: $(SRCDIR)/readInput.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)

$(OBJDIR)/preProc.o: $(SRCDIR)/preProc.c $(HEADER)
	$(CPP) $(HOSTFLAGS) -c -o $@ $< $(USEDPUS)


clean:
	rm $(OBJDIR)/*
	rm bin/*
