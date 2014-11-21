
SRC=de
TRG=en
LANGPAIR = $(SRC)-$(TRG)
CORPUS = Europarl

SMTTOOLS = ${HOME}/work/projects/SMT/tools
MOSESHOME = ${SMTTOOLS}/mosesdecoder
KENLMHOME = $(SMTTOOLS)/kenlm

DOCENTHOME = ../..
EXTERNAL = ${DOCENTHOME}/DEBUG/external/Install


test.000000000.xml:
	../../DEBUG/lcurve-docent -d SelectedWordLM -n input.xml config.xml test


tm: $(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx
lm: $(CORPUS)/${CORPUS}.${TRG}.kenlm
selected-words-lm: $(CORPUS).${LANGPAIR}.${TRG}.selected.kenlm


fetch-corpus: 	$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz \
		$(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz \
		$(CORPUS)/$(LANGPAIR)/ids.gz \
		$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz \
		$(CORPUS)/raw/$(SRC) \
		$(CORPUS)/raw/$(TRG)

$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz $(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz $(CORPUS)/$(LANGPAIR)/ids.gz:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/$(LANGPAIR)/$(notdir $@) )

$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/${LANGPAIR}/model/$(notdir $@) )

$(CORPUS)/raw/$(SRC) $(CORPUS)/raw/$(TRG):
	wget http://opus.lingfil.uu.se/$(CORPUS)/$(notdir $@).raw.tar.gz
	tar -xf $(notdir $@).raw.tar.gz
	rm -f $(notdir $@).raw.tar.gz


$(CORPUS).${LANGPAIR}.${TRG}.selected.txt: 	$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz \
						$(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz \
						$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz \
						$(CORPUS)/$(LANGPAIR)/ids.gz
	${MAKE} $(CORPUS)/raw/$(SRC) $(CORPUS)/raw/$(TRG)
	./select-words.pl $^ $(CORPUS)/raw > $@

$(CORPUS).${LANGPAIR}.${TRG}.selected.kenlm: %.kenlm: %.txt
	${KENLMHOME}/bin/lmplz -o 3 < $< > $(@:kenlm=arpa)
	${KENLMHOME}/bin/build_binary $(@:kenlm=arpa) $@
	rm -f $(@:kenlm=arpa)

$(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/$(LANGPAIR)/model/phrase-table-filtered.gz )
	gzip -cd $(@:.binphr.idx=-filtered.gz) | LC_ALL=C sort |\
	${EXTERNAL}/moses_external/bin/processPhraseTable \
		-ttable 0 0 - -nscores 4 -alignment-info -out $(@:.binphr.idx=)
	rm -f $(@:.binphr.idx=-filtered.gz)

$(CORPUS)/${CORPUS}.${TRG}.kenlm:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/mono/$(patsubst %.kenlm,%.gz,$(notdir $@)) )
	gunzip $(@:kenlm=gz)
	${KENLMHOME}/bin/lmplz -o 3 < $(@:.kenlm=) > $(@:kenlm=arpa)
	${KENLMHOME}/bin/build_binary $(@:kenlm=arpa) $@
	rm -f $(@:.kenlm=) $(@:kenlm=arpa)