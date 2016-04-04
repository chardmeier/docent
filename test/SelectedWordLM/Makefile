
SRC=de
TRG=en
LANGPAIR   = $(SRC)-$(TRG)
space      = $(empty) $(empty)
SORTEDLANG = $(subst  $(space),-,$(sort ${SRC} ${TRG}))



CORPUS = Europarl

SMTTOOLS = ${HOME}/work/projects/SMT/tools
MOSESHOME = ${SMTTOOLS}/mosesdecoder
KENLMHOME = $(SMTTOOLS)/kenlm

DOCENTHOME = ../..
EXTERNAL = ${DOCENTHOME}/DEBUG/external/Install

DATA = ../data
NEWS12 = $(DATA)/newstest2012/newstest2012
# NEWS12 = $(DATA)/newstest2012/newstest2012-small

SELECTED_POS = NOUN
SRCTAGGER = hunpos-tag $(SRC).model


.PHONY: news12-base news12-pos news12-verb news12-2pos news12-swlm

news12-base: news12-base.$(LANGPAIR).txt news12-base.$(LANGPAIR).134217728.eval
news12-swlm: news12-swlm.$(LANGPAIR).txt news12-swlm.$(LANGPAIR).134217728.eval
news12-pos: news12-${SELECTED_POS}.$(LANGPAIR).txt news12-${SELECTED_POS}.$(LANGPAIR).134217728.eval
news12-2pos: news12-2pos.$(LANGPAIR).txt news12-2pos.$(LANGPAIR).134217728.eval

news12-verb:
	${MAKE} SELECTED_POS=VERB news12-pos



.PHONY: smt-model
smt-model: lm tm


.PHONY: test
test: test.134217728.xml


# get the plain text translations from XML files

%.txt: %.134217728.xml
	grep '<seg' $< |\
	sed 's/^ *>//;s/<[^>]*>//g;' > $@



news12-base.$(LANGPAIR).134217728.xml: \
			$(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx \
			$(CORPUS)/${CORPUS}.${TRG}.kenlm
	$(DOCENTHOME)/DEBUG/lcurve-docent \
		-n ${NEWS12}.$(SRC).src.xml \
		config-base.$(LANGPAIR).xml \
		news12-base.$(LANGPAIR)

news12-VERB.$(LANGPAIR).134217728.xml:
	${MAKE} SELECTED_POS=VERB news12-pos

news12-${SELECTED_POS}.$(LANGPAIR).134217728.xml: $(notdir $(NEWS12)).$(SRC) \
			$(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx \
			$(CORPUS)/${CORPUS}.${TRG}.kenlm \
			$(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.${SELECTED_POS}.kenlm
	${MAKE} selected-pos-lm
	$(DOCENTHOME)/DEBUG/lcurve-docent \
		-m $< ${NEWS12}.$(SRC).src.xml \
		config-${SELECTED_POS}.$(LANGPAIR).xml \
		news12-${SELECTED_POS}.$(LANGPAIR)

news12-2pos.$(LANGPAIR).134217728.xml: $(notdir $(NEWS12)).$(SRC) \
			$(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx \
			$(CORPUS)/${CORPUS}.${TRG}.kenlm
	$(MAKE) SELECTED_POS=NOUN $(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.NOUN.kenlm
	$(MAKE) SELECTED_POS=VERB $(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.VERB.kenlm
	$(DOCENTHOME)/DEBUG/lcurve-docent \
		-m $< ${NEWS12}.$(SRC).src.xml \
		config-2pos.$(LANGPAIR).xml \
		news12-2pos.$(LANGPAIR)



news12-swlm.$(LANGPAIR).134217728.xml: $(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.selected.kenlm \
			$(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx \
			$(CORPUS)/${CORPUS}.${TRG}.kenlm
	$(DOCENTHOME)/DEBUG/lcurve-docent \
		-n ${NEWS12}.$(SRC).src.xml \
		config-swlm.$(LANGPAIR).xml \
		news12-swlm.$(LANGPAIR)




## convert input into MMAX format (with POS level annotation)

$(notdir $(NEWS12)).$(SRC): $(NEWS12).$(SRC).src.xml
	rm -fr $@
	$(DOCENTHOME)/scripts/mmax/mmax-tagged-xml-import.sh $< $@ $(SRC).model






news12%.eval: news12%.xml
	$(DOCENTHOME)/scripts/eval/mteval-v13.pl \
		-s $(NEWS12).$(SRC).src.xml \
		-r $(NEWS12).$(TRG).ref.xml \
		-t $< > $@


# simple stupid test config and input

test.134217728.xml:
	$(DOCENTHOME)/DEBUG/lcurve-docent -d SelectedWordLM -n input.xml config-test.xml test


.PHONY: tm lm selected-words-lm selected-pos-lm fetch-corpus

tm: $(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx
lm: $(CORPUS)/${CORPUS}.${TRG}.kenlm
selected-words-lm: $(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.selected.kenlm
selected-pos-lm: $(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.${SELECTED_POS}.kenlm


fetch-corpus: 	$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz \
		$(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz \
		$(CORPUS)/$(LANGPAIR)/ids.gz \
		$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz \
		$(CORPUS)/raw/$(SRC) \
		$(CORPUS)/raw/$(TRG)

$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz $(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz $(CORPUS)/$(LANGPAIR)/ids.gz:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/$(SORTEDLANG)/$(notdir $@) )

$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz $(CORPUS)/$(LANGPAIR)/aligned.intersect.gz:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/${SORTEDLANG}/model/$(notdir $@) )

$(CORPUS)/raw/$(SRC) $(CORPUS)/raw/$(TRG):
	wget http://opus.lingfil.uu.se/$(CORPUS)/$(notdir $@).raw.tar.gz
	tar -xf $(notdir $@).raw.tar.gz
	rm -f $(notdir $@).raw.tar.gz


$(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.selected.txt: 	\
	$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz \
	$(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz \
	$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz \
	$(CORPUS)/$(LANGPAIR)/ids.gz
	${MAKE} $(CORPUS)/raw/$(SRC) $(CORPUS)/raw/$(TRG)
	./select-words.pl -l 4 -d SPEAKER $^ $(CORPUS)/raw > $@

$(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.${SELECTED_POS}.txt: 	\
	$(CORPUS)/$(LANGPAIR)/c.pos.$(SRC).gz \
	$(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz \
	$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz \
	$(CORPUS)/$(LANGPAIR)/ids.gz
	${MAKE} $(CORPUS)/raw/$(SRC) $(CORPUS)/raw/$(TRG)
	./select-words.pl -p ${SELECTED_POS} -d SPEAKER $^ $(CORPUS)/raw > $@

#	$(CORPUS)/$(LANGPAIR)/aligned.intersect.gz \



$(CORPUS)/$(LANGPAIR)/c.pos.$(SRC).gz: $(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz
	zcat $< |\
	sed "s/$$/\n/" | tr ' ' "\n" | \
	${SRCTAGGER} | \
	tr "\t" '/' | tr "\n" ' ' | sed "s/  /\n/g" |\
	gzip -c > $@


# $(CORPUS)/$(CORPUS).${LANGPAIR}.${TRG}.selected.kenlm: %.kenlm: %.txt
%.kenlm: %.txt
	mkdir -p $(dir $@)
	${KENLMHOME}/bin/lmplz -o 7 < $< > $(@:kenlm=arpa)
	${KENLMHOME}/bin/build_binary $(@:kenlm=arpa) $@
	rm -f $(@:kenlm=arpa)

$(CORPUS)/$(LANGPAIR)/phrase-table.binphr.idx:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/$(SORTEDLANG)/model/phrase-table-filtered.gz )
	if [ "${LANGPAIR}" != "${SORTEDLANG}" ]; then \
	  zcat $(dir $@)phrase-table-filtered.gz | \
	  perl ${DOCENTHOME}/scripts/train/reverse_phrase_table.pl | \
	  gzip -c > $(dir $@)phrase-table-filtered-rev.gz; \
	  mv $(dir $@)phrase-table-filtered-rev.gz $(dir $@)phrase-table-filtered.gz; \
	fi
	gzip -cd $(@:.binphr.idx=-filtered.gz) | LC_ALL=C sort |\
	${EXTERNAL}/moses_external/bin/processPhraseTable \
		-ttable 0 0 - -nscores 4 -alignment-info -out $(@:.binphr.idx=)
	rm -f $(@:.binphr.idx=-filtered.gz)

$(CORPUS)/${CORPUS}.${TRG}.kenlm:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/mono/$(patsubst %.kenlm,%.gz,$(notdir $@)) )
	gunzip $(@:kenlm=gz)
	${KENLMHOME}/bin/lmplz -o 5 < $(@:.kenlm=) > $(@:kenlm=arpa)
	${KENLMHOME}/bin/build_binary $(@:kenlm=arpa) $@
	rm -f $(@:.kenlm=) $(@:kenlm=arpa)
