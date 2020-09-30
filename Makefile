.PHONY: lint
lint:
	flake8 && cpplint src/*

.PHONY: test
test:
	pytest