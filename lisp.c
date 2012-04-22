
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void* CELL;

#define NIL (0)
#define LIST (0)
#define NUMBER (1)
#define FUNCTION (2)
#define STRING (3)
#define T (4)
#define F (5)
#define LAMBDA (6)

#define MAX_SIZE (10000)
#define MAX_LABEL_LEN (64)

typedef struct {
	CELL car;
	CELL cdr;
} CONS;

typedef struct {
	int type;
	char value[256];
} SYMBOL;

typedef struct {
	char label[MAX_LABEL_LEN];
	SYMBOL* value;
} BIND;

typedef struct {
	char label[MAX_LABEL_LEN];
	int arg_count;
	SYMBOL* arg;
	SYMBOL* body;
} FUNC;

CONS cons_heap[MAX_SIZE];
SYMBOL heap[MAX_SIZE];
BIND bind[MAX_SIZE];
FUNC func[MAX_SIZE];
BIND stack[MAX_SIZE];
int cons_heap_ptr;
int heap_ptr;
int bind_ptr;
int func_ptr;
int stack_ptr;

SYMBOL* match(SYMBOL* func, CONS* cdr);
SYMBOL* car(CONS* cons);
CONS* cdr(CONS* cons);
SYMBOL* eval(SYMBOL* symb);
void gc(void);
void dump_stack(void);

SYMBOL* allocate_symbol(void)
{
	if ((float)heap_ptr / (float)MAX_SIZE >= 0.8){
		gc();
	}
	return (&heap[heap_ptr++]);
}

CONS* allocate_cons(void)
{
	if ((float)cons_heap_ptr / (float)MAX_SIZE >= 0.8){
		gc();
	}
	return (&cons_heap[cons_heap_ptr]);
}

BIND* allocate_stack(void)
{
	if (stack_ptr >= MAX_SIZE){
		printf("stack_over_flow\n");
		exit(-1);
	}
	return (&stack[stack_ptr++]);
}

void bind_set(char label[], SYMBOL* value)
{
	int i;

	for (i = 0; i < bind_ptr; i++){
		if (strcmp(bind[i].label, label) == 0){
			bind[i].value = value;
			return;
		}
	}
	strcpy(bind[bind_ptr].label, label);
	bind[bind_ptr].value = value;
	bind_ptr++;
}

BIND* bind_find(char label[])
{
	int i;

	for (i = 0; i < bind_ptr; i++){
		if (strcmp(bind[i].label, label) == 0){
			return (&bind[i]);
		}
	}

	return (NIL);
}

int _get_symbol_index(SYMBOL* target)
{
	int i;

#if 0
	for (i = 0; i < heap_ptr; i++){
		if (&heap[i] == target){
			printf("%d %d\n", i, target - heap);
			return (i);
		}
	}
#endif
	if ((target - heap) < MAX_SIZE && (target - heap) >= 0){
		return (target - heap);
	}
	printf("nanikagaokasii\n");
	printf("%d\n", target - heap);
	exit(-1);
	return (-1);
}

int _get_cons_index(CONS* target)
{
	int i;
#if 0
	for (i = 0; i < cons_heap_ptr; i++){
		if (&cons_heap[i] == target){
			printf("%d %d\n", i, target - cons_heap);
			return (i);
		}
	}
#endif
	if ((target - cons_heap) < MAX_SIZE && (target - cons_heap) >= 0){
		return (target - cons_heap);
	}
	printf("nanikagaokasii cons");
	return (-1);
}

int check_bind(char check[])
{
	int i;
	int index;

	for (i = stack_ptr - 1; i >= 0; i--){
		if (stack[i].value != NULL){
#ifdef DEBUG
			printf("%d:stack=%s\n", i, stack[i].label);
			print_symbol(stack[i].value);
			puts("");
#endif
			index = _get_symbol_index(stack[i].value);
			check[index] = 1;
		}
	}
	for (i = 0; i < bind_ptr; i++){
		printf("bind=%s\n", bind[i].label);
		index = _get_symbol_index(bind[i].value);
		check[index] = 1;
	}
	
	return (0);
}

int check_func(char check[])
{
	int i;
	int index;

	for (i = 0; i < func_ptr; i++){
		puts(func[i].label);
		index = _get_symbol_index(func[i].arg);
		check[index] = 1;
		index = _get_symbol_index(func[i].body);
		check[index] = 1;
	}

	return (0);
}

int checking(char check[], char check_cons[], int i)
{
	SYMBOL* cur;
	CONS* _car;

	cur = &heap[i];
	if (cur->type == LIST){
		_car = (CONS *)(cur->value);
		while (_car->car != NIL){
			int index, index_cons;
			index = _get_symbol_index(car(_car));
			index_cons = _get_cons_index(cdr(_car));
			if (check[index] == 0){
				check[index] = 1;
				checking(check, check_cons, index);
			}
			if (check_cons[index_cons] == 0){
				check_cons[index_cons] = 1;
			}
			_car = cdr(_car);
		}
	}
	else if (cur->type == LAMBDA){
		FUNC* func;
		int index;

		func = (FUNC *)(cur->value);
		index = _get_symbol_index(func->arg);
		if (check[index] == 0){
			check[index] = 1;
			checking(check, check_cons, index);
		}
		index = _get_symbol_index(func->body);
		if (check[index] == 0){
			check[index] = 1;
			checking(check, check_cons, index);
		}
	}
	return (0);
}

void replace_pointer(char* old, char* new)
{
	int i;
	
	for (i = 0; i < heap_ptr; i++){
		if (heap[i].type == LIST){
			if (((CONS *)(heap[i].value))->car == (void *)old){
				((CONS *)(heap[i].value))->car = (void *)new;
			}
			if (((CONS *)(heap[i].value))->cdr == (void *)old){
				((CONS *)(heap[i].value))->cdr = (void *)new;
			}
		}
		else if (heap[i].type == LAMBDA){
			FUNC* func;
			func = (FUNC *)(heap[i].value);
			if (func->arg == (SYMBOL *)old){
				func->arg = (SYMBOL *)new;
			}
			if (func->body == (SYMBOL *)old){
				func->body = (SYMBOL *)new;
			}
		}
	}
	for (i = 0; i < cons_heap_ptr; i++){
		if (cons_heap[i].car == (void *)old){
			cons_heap[i].car = (void *)new;
		}
		if (cons_heap[i].cdr == (void *)old){
			cons_heap[i].cdr = (void *)new;
		}
	}
	for (i = 0; i < func_ptr; i++){
		if (func[i].arg == (SYMBOL *)old){
			func[i].arg = (SYMBOL *)new;
		}
		if (func[i].body == (SYMBOL *)old){
			func[i].body = (SYMBOL *)new;
		}
	}
	for (i = 0; i < bind_ptr; i++){
		if (bind[i].value == (SYMBOL *)old){
			bind[i].value = (SYMBOL *)new;
		}
	}
	for (i = 0; i < stack_ptr; i++){
		if (stack[i].value == (SYMBOL *)old){
			stack[i].value = (SYMBOL *)new;
		}
	}
}

void defrag(char check[MAX_SIZE], char heap[], int size, int* heap_ptr)
{
	int new_ptr, old_ptr;

	new_ptr = old_ptr = 0;
	while (old_ptr < *heap_ptr){
		if (check[old_ptr] == 1){
			memcpy(&heap[new_ptr * size], &heap[old_ptr * size], size);
			replace_pointer(&heap[old_ptr * size], &heap[new_ptr * size]);
			new_ptr++;
		}
		old_ptr++;
	}
	*heap_ptr = new_ptr;
}

void gc(void)
{
	char check[MAX_SIZE];
	char check_cons[MAX_SIZE];
	int new_ptr;
	int old_ptr;
	int i;
	
	printf("symbol_heap=%d cons_heap %d\n", heap_ptr, cons_heap_ptr);
	printf("start gc\n");
	memset(check, 0, sizeof(check));
	memset(check_cons, 0, sizeof(check_cons));
	check_bind(check);
	check_func(check);
	for (i = 0; i < MAX_SIZE; i++){
		if (check[i] == 1){
			checking(check, check_cons, i);
		}
	}
	defrag(check, (char *)heap, sizeof(SYMBOL), &heap_ptr);
	defrag(check_cons, (char *)cons_heap, sizeof(CONS), &cons_heap_ptr);
	for (i = heap_ptr; i < MAX_SIZE; i++){
		memset(&heap[i], 0, sizeof(SYMBOL));
	}
	puts("end gc");
	printf("symbol_heap=%d cons_heap %d\n", heap_ptr, cons_heap_ptr);
}

SYMBOL* bind_match(char label[])
{
	int i;

	for (i = stack_ptr - 1; i >= 0; i--){
		if (strcmp(label, stack[i].label) == 0){
			return (stack[i].value);
		}
	}
	for (i = 0; i < bind_ptr; i++){
		if (strcmp(label, bind[i].label) == 0){
			return (bind[i].value);
		}
	}

	printf("%s\n", label);
	puts("no value");
	exit(1);
	return (NULL);
}

SYMBOL* const_match(char label[])
{
	SYMBOL* result;

	result = NIL;
	if (strcmp(label, "t") == 0){
		result = allocate_symbol();
		result->type = T;
	}
	else if (strcmp(label, "nil") == 0){
		result = allocate_symbol();
		result->type = F;
	}

	return (result);
}

BIND* bind_stack_find(char label[])
{
	int i;

	for (i = stack_ptr - 1; i >= 0; i--){
		if (strcmp(stack[i].label, label) == 0){
			return (&stack[i]);
		}
	}

	return (NIL);
}

void bind_stack_push(char label[], SYMBOL* value)
{
	BIND* bind = allocate_stack();

	strcpy(bind->label, label);
	bind->value = value;
}

void bind_stack_start()
{
	BIND* bind = allocate_stack();

	strcpy(bind->label, "__STACKSTART__");
	bind->value = NIL;
}

void bind_stack_pop(int pop_count)
{
	stack_ptr -= pop_count;
}

void bind_stack_end(void)
{
	while (stack_ptr >= 0 && stack[stack_ptr - 1].value != NIL){
		stack_ptr--;
	}
	stack_ptr--;
}

FUNC* func_match(char label[])
{
	int i;

	for (i = 0; i < func_ptr; i++){
		if (strcmp(label, func[i].label) == 0){
			return (&func[i]);
		}
	}

	printf("no function\n");
	exit(1);
	return (NIL);
}

void func_set(char label[], SYMBOL* arg, SYMBOL* body)
{
	FUNC* f = &func[func_ptr++];
	CONS* cur;
	int count;

	strcpy(f->label, label);
	f->arg = arg;
	f->body = body;

	cur = (CONS *)(arg->value);
	count = 0;
	while (cur->car != NIL){
		count++;
		cur = cdr(cur);
	}
	f->arg_count = count;
}

void dump_func(void)
{
	int i;

	for (i = 0; i < func_ptr; i++){
		printf("%s:", func[i].label);
		print_symbol(func[i].arg);
		print_symbol(func[i].body);
		puts("");
	}
}

void dump_symbol(void)
{
	int i;

	puts("nadeko");
	for (i = 0; i < heap_ptr; i++){
#ifdef DEBUG
		printf("%d: ", i);
		print_symbol(&heap[i]);
		puts("");
#endif
	}
}
void dump_stack(void)
{
	int i;

	for (i = 0; i < stack_ptr; i++){
		printf("%s:", stack[i].label);
		if (stack[i].value != NIL){
			print_symbol(stack[i].value);
		}
		puts("");
	}
}

SYMBOL* funcall(FUNC* f, CONS* arg)
{
	CONS* cur_real_arg;
	CONS* cur_vart_arg;
	SYMBOL* result;
	SYMBOL temp;
	
#ifdef DEBUG_FUNCSTEP
	printf("in %s", f->label);
	print_symbol(f->arg);
	print_symbol(f->body);
	temp.type = LIST;
	((CONS *)(temp.value))->car = arg->car;
	((CONS *)(temp.value))->cdr = arg->cdr;
	puts("");
	printf("arg=");
	print_symbol(&temp);
	puts("");
#endif
	cur_real_arg = arg;
	cur_vart_arg = (CONS *)((f->arg)->value);
	bind_stack_start();
	while (cur_real_arg->car != NIL && cur_vart_arg->car != NIL){
		SYMBOL* label;
		SYMBOL* val;
		label = car(cur_vart_arg);
		val = car(cur_real_arg);
		bind_stack_push(label->value, eval(val));
		cur_vart_arg = cdr(cur_vart_arg);
		cur_real_arg = cdr(cur_real_arg);
	}
	result = eval(f->body);
	bind_stack_end();
#ifdef DEBUG_FUNCSTEP
	dump_stack();
	printf("out %s\n", f->label);
#endif
	
	return (result);
}

int cons_length(CONS*cons)
{
	CONS* cur;
	int result;

	cur = cons;
	result = 0;
	while (cur->car != NIL){
		result++;
		cur = cdr(cur);
	}
	return (result);
}

SYMBOL* lambda(CONS* cons)
{
	SYMBOL* result;
	FUNC* func;

	result = allocate_symbol();
	result->type = LAMBDA;
	func = (FUNC *)(result->value);
	func->arg = car(cons);
	func->body = car(cdr(cons));
	func->arg_count = cons_length(cons);

	return (result);
}

SYMBOL* car(CONS* cons)
{
	return ((SYMBOL *)(cons->car));
}

CONS* cdr(CONS* cons)
{
	return ((CONS *)(cons->cdr));
}

SYMBOL* eval(SYMBOL* symb)
{
	SYMBOL* _car;
	CONS* _cdr;
	SYMBOL* result;

	
	switch (symb->type){
	  case LIST:
	  	_car = car((CONS *)(symb->value));
		if (((CONS *)(symb->value))->car == NIL){
			result = allocate_symbol();
			result->type = F;
			return (result);
		}
		_cdr = cdr((CONS *)(symb->value));
		return(match(_car, _cdr));
	  case NUMBER:
	  case STRING:
	  case T:
	  case F:
	  	return (symb);
	  case FUNCTION:
	  	result = const_match(symb->value);
		if (result != NIL){
			return (result);
		}
	  	return (bind_match(symb->value));
	  default:
	  	printf("InvalitType :  %d\n", symb->type);
		exit(0);
		return (NIL);
	}
	return (NIL);
}

SYMBOL* null(CONS* _cdr)
{
	SYMBOL* result;
	SYMBOL* cur;
	
	result = allocate_symbol();
	cur = eval(car(_cdr));
	if (cur->type == LIST){
		if (((CONS *)(cur->value))->car == NIL){
			result->type = T;
		}
		else {
			result->type = F;
		}
	}
	else if (cur->type == F){
		result->type = T;
	}
	else {
		result->type = F;
	}

	return (result);
}

SYMBOL* defun(CONS* _cdr)
{
	SYMBOL* name;
	SYMBOL* arg;
	SYMBOL* body;
	SYMBOL* result;

	name = car(_cdr);
	arg = car(cdr(_cdr));
	body = car(cdr(cdr(_cdr)));

	func_set(name->value, arg, body);

	result = allocate_symbol();
	result->type = F;
	return result;
}


SYMBOL* setq(CONS* _cdr)
{
	SYMBOL* val;
	SYMBOL* set;
	SYMBOL* result;

	result = allocate_symbol();
	result->type = F;

	val = car(_cdr);
	set = car(cdr(_cdr));

	if (stack_ptr == 0){
		bind_set(val->value, eval(set));
	}
	else {
		BIND* t;
		if ((t = bind_stack_find(val->value)) != NIL){
			t->value = eval(set);
		}
		else if ((t = bind_find(val->value)) != NIL){
			t->value = eval(set);
		}
		else {
			bind_stack_push(val->value, eval(set));
		}
	}

	return (result);
}

SYMBOL* cond(CONS* _cdr)
{
	SYMBOL* conditon;
	SYMBOL* execute;
	CONS* cur = _cdr;
	
	while (cur->car != NIL){
		conditon = car((CONS *)(car(cur)->value));
		if ((eval(conditon))->type == T){
			execute = car(cdr((CONS *)(car(cur)->value)));
			return (eval(execute));
		}
		cur = cdr(cur);
	}
	
	printf("Error Cond\n");
	exit(0);
	return (NIL);
}

SYMBOL* _if(CONS* _cdr)
{
	SYMBOL* conditon;

	conditon = eval(car(_cdr));
	if (conditon->type == T){
		return (eval(car(cdr(_cdr))));
	}
	else {
		return (eval(car(cdr(cdr(_cdr)))));
	}
	puts("if: nanikaga");
	return (NIL);
}

SYMBOL* plus(CONS* _cdr)
{
	SYMBOL* _car;
	SYMBOL* result = allocate_symbol();
	int num;
	
	result->type = NUMBER;
	*(int *)(result->value) = 0;
	_car = car(_cdr);
	_cdr = cdr(_cdr);
	while (_car != NIL) {
		if (_car->type == STRING){
			return (0);
		}
		else {
			num = *(int *)(eval(_car)->value);
		}
#ifdef DEBUG
		printf("num=%d\n", num);
#endif
		*(int *)(result->value) += num;
		_car = car(_cdr);
		_cdr = cdr(_cdr);
	} ;
#ifdef DEBUG
	printf("%d\n", *(int *)(result->value));
#endif
	return (result);
}

SYMBOL* eq(CONS* _cdr)
{
	SYMBOL* a;
	SYMBOL* b;
	SYMBOL* result = allocate_symbol();

	a = eval(car(_cdr));
	b = eval(car(cdr(_cdr)));
#ifdef DEBUG
	print_symbol(a);
	print_symbol(b);
#endif

	if (a->type == b->type && memcmp(a->value, b->value, 256) == 0){
		result->type = T;
	}
	else {
		result->type = F;
	}
#ifdef DEBUG
	print_symbol(result);
#endif
	return (result);
}

SYMBOL* minus(CONS* _cdr)
{
	SYMBOL* _car;
	SYMBOL* result = allocate_symbol();
	int num;
	
	result->type = NUMBER;
	_car = car(_cdr);
	_cdr = cdr(_cdr);
	num = *(int *)(eval(_car)->value);
	*(int *)(result->value) = num;
	_car = car(_cdr);
	_cdr = cdr(_cdr);
	while (_car != NIL) {
		if (_car->type == STRING){
			return (0);
		}
		else if (_car->type == LIST){
			num = *(int *)(eval(_car)->value);
		}
		else {
			num = *(int *)(_car->value);
		}
#ifdef DEBUG
		printf("num=%d\n", num);
#endif
		*(int *)(result->value) -= num;
		_car = car(_cdr);
		_cdr = cdr(_cdr);
	} ;
#ifdef DEBUG
	printf("%d\n", *(int *)(result->value));
#endif
	return (result);
}

SYMBOL* multi(CONS* _cdr)
{
	SYMBOL* _car;
	SYMBOL* result = allocate_symbol();
	int num;
	
	result->type = NUMBER;
	*(int *)(result->value) = 1;
	_car = car(_cdr);
	_cdr = cdr(_cdr);
	while (_car != NIL) {
		if (_car->type == STRING){
			return (0);
		}
		else {
			num = *(int *)(eval(_car)->value);
		}
#ifdef DEBUG
		printf("num=%d\n", num);
#endif
		*(int *)(result->value) *= num;
		_car = car(_cdr);
		_cdr = cdr(_cdr);
	} ;
#ifdef DEBUG
	printf("%d\n", *(int *)(result->value));
#endif
	return (result);
}

SYMBOL* quote(CONS* _cdr)
{
	return (car(_cdr));
}

CONS* _cons(SYMBOL* a, CONS* b)
{
	CONS* result;
	
	result = &cons_heap[cons_heap_ptr++];
	result->car = (CELL)a;
	result->cdr = (CELL)b;

	return (result);
}

SYMBOL* cons(CONS* _cdr)
{
	SYMBOL* result;
	CONS* cur;

	result = allocate_symbol();
	result->type = LIST;
	cur = _cons(eval(car(_cdr)), (CONS *)(eval(car(cdr(_cdr)))->value));
	((CONS *)(result->value))->car = cur->car;
	((CONS *)(result->value))->cdr = cur->cdr;

	return (result);
}

SYMBOL* match(SYMBOL* func, CONS* _cdr)
{
	if (func->type != FUNCTION){
		printf("InvalitFunction");
		exit(0);
		return (NIL);
	}
	else if (strcmp(func->value, "plus") == 0){
		return (plus(_cdr));
	}
	else if (strcmp(func->value, "minus") == 0){
		return (minus(_cdr));
	}
	else if (strcmp(func->value, "multi") == 0){
		return (multi(_cdr));
	}
	else if (strcmp(func->value, "quote") == 0){
		return (quote(_cdr));
	}
	else if (strcmp(func->value, "eq") == 0){
		return (eq(_cdr));
	}
	else if (strcmp(func->value, "null") == 0){
		return (null(_cdr));
	}
	else if (strcmp(func->value, "eval") == 0){
		return (eval(eval(car(_cdr))));
	}
	else if (strcmp(func->value, "setq") == 0){
		return (setq(_cdr));
	}
	else if (strcmp(func->value, "defun") == 0){
		return (defun(_cdr));
	}
	else if (strcmp(func->value, "lambda") == 0){
		return (lambda(_cdr));
	}
	else if (strcmp(func->value, "funcall") == 0){
		FUNC* f;
		f = (FUNC *)((eval(car(_cdr)))->value);
		return (funcall(f, cdr(_cdr)));
	}
	else if (strcmp(func->value, "car") == 0){
		return (car((CONS *)(eval(car(_cdr))->value)));
	}
	else if (strcmp(func->value, "cond") == 0){
		return (cond(_cdr));
	}
	else if (strcmp(func->value, "if") == 0){
		return (_if(_cdr));
	}
	else if (strcmp(func->value, "cons") == 0){
		return (cons(_cdr));
	}
	else if (strcmp(func->value, "gc") == 0){
		gc();
		return (NIL);
		}
	else if (strcmp(func->value, "cdr") == 0){
		SYMBOL* result = allocate_symbol();
		CONS* cddr;

		result->type = LIST;
		cddr = cdr((CONS *)(eval(car(_cdr))->value));
		((CONS *)(result->value))->car = cddr->car;
		((CONS *)(result->value))->cdr = cddr->cdr;
		return (result);
	}
	else {
		FUNC* f;
		f = func_match(func->value);
		return (funcall(f, _cdr));
	}
}

int skip(char* input, int index)
{
	int sp = 0;

	while (1){
		printf("\"%c\"\n", input[index]);
		if (input[index] == '('){
			sp++;
		}
		else if (input[index] == ')'){
			sp--;
		}
		if (sp <= 0){
			if (input[index] == ')' || input[index] == ' ' || input[index] == '\0'){
				break;
			}
		}
		index++;
	}

	return (index);
}

SYMBOL* shell(char* input, int* end_index)
{
	SYMBOL* result = allocate_symbol();
	CONS* cons;
	CONS* temp;
	int index = 0;
	int end;
	
	while (isspace(input[index]) || input[index] == ')') {
		index++;
	}
	if (input[index] == '('){
		index++;
		result->type = LIST;
		/* *(unsigned int *)(result->value) = &cons_heap[cons_heap_ptr++];*/
		/* *(int *)(result->value) = &cons_heap[cons_heap_ptr++]; */
		cons = (CONS *)(result->value);
		cons->car = NIL;
		cons->cdr = NIL;
		while (input[index] != ')' && input[index] != '\0'){
			while (input[index] == ' '){
				index++;
			}
			(cons->car) = shell(input + index, &end);
#ifdef DEBUG
			printf("value=%d\n", *(int *)(((SYMBOL *)(cons->car))->value));
#endif
			temp = &cons_heap[cons_heap_ptr++];
			temp->car = NIL;
			temp->cdr = NIL;
			(cons->cdr) = temp;
			cons = temp;
			//index++;
			index += end;
#ifdef DEBUG
			printf("e='%c' %d\n", input[index], index);
			printf("end='%c'\n", input[index]);
#endif
		}
	}
	else if (isdigit(input[index]) || input[index] == '-'){
		int num = atoi(input + index);
#ifdef DEBUG
		printf("%d\n", num);
#endif
		result->type = NUMBER;
		*(int *)(result->value) = num;
#ifdef DEBUG
		printf("%d\n", *(int *)(result->value));
#endif
		while (isdigit(input[index]) || input[index] == '-'){
			index++;
		}
	}
	else if (isalpha(input[index])){
		int copy_index;
		result->type = FUNCTION;

		copy_index = 0;
		while (isalpha(input[index + copy_index])){
			result->value[copy_index] = input[index + copy_index];
			copy_index++;
		}
		result->value[copy_index] = '\0';
		index += copy_index;
	}
	if (result->type == LIST){
		while(input[index] != ')'){
			index++;
		}
		index++;
	}
	while (isspace(input[index])){
		index++;
	}
	*end_index = index;
	return (result);
}

int print_symbol(SYMBOL* symb)
{
	CONS* cons;

	if (symb == NIL){
		puts("nil");
	}

	if (symb->type == LIST){
		printf("( ");
		cons = (CONS *)symb->value;
		while (cons->car != NIL){
			print_symbol(car(cons));
			cons = cdr(cons);
		}
		printf(") ");
	}
	else if (symb->type == NUMBER){
		printf("%d ", *(int *)(symb->value));
	}
	else if (symb->type == FUNCTION) {
		printf("%s ", symb->value);
	}
	else if (symb->type == T){
		printf("t ");
	}
	else if (symb->type == F){
		printf("nil ");
	}
	else if (symb->type == LAMBDA){
		printf("( lambda ");
		print_symbol(((FUNC *)(symb->value))->arg);
		print_symbol(((FUNC *)(symb->value))->body);
		printf(") ");
	}
	return (0);
}

int main(void)
{
	char str[256];
	int i;
	int end;
	SYMBOL* s;

	i = 0;
	while (str[i] != '\0'){
		if (str[i] == '\n'){
			str[i] = '\0';
		}
		i++;
	}
#if 1
	s = shell("(multi 1 2 3)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(plus (car (quote (1 2))) 2 4)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(plus 1 (car (cdr (quote (1 2)))))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(eq 1 2)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(eval (plus 1 2))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(cond ((eq 1 0) (plus 1 2)) ((eq 1 1) (plus 1 3)))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(setq x 1)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(eq x 1)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun inc (x) (plus x 1))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(inc 10)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun sum (x) (cond ((eq x 1) 1) ((eq 1 1) (plus x (sum (minus x 1))))))", &end);
	print_symbol(s);
	print_symbol(eval(s));
	puts("");
	s = shell("(sum 10)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(setq x (quote (1 2 3 4))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(null (quote (1 2 3)))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun list (x) (cond ((null x) 0) ((eq 1 1) (plus (car x) (list (cdr x))))))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(list (quote (1 2 3 7)))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(cons 123 (quote (1 2 3 7)))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(setq y (sum 10))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(setq x (lambda (x y) (plus x y)))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(funcall x 1 3)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun test (x y z) (funcall x y z))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(test (lambda (x y) (plus x y)) 10 2)", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun inclist (x) (cond ((null x) nil) ((eq 1 1) (cons (plus (car x) 1) (inclist (cdr x))))))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(inclist (quote (1 2 3)))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("()", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(    plus  5  (plus 2 3)  ) ", &end);
	print_symbol(s);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun nadeko  () (plus 1 2)) ", &end);
	print_symbol(s);
	print_symbol(eval(s));
	puts("");
	s = shell("(nadeko)", &end);
	print_symbol(eval(s));
	s = shell("(inclist (quote (1 2 3)))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun concat (x y) (if (null x) y (cons (car x) (concat (cdr x) y))))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("y", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(if (null nil) y (plus 2 3))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(defun map (fun lst) (cond ((null lst) nil) (t (cons (funcall fun (car lst)) (map fun (cdr lst)))))", &end);
	print_symbol(eval(s));
	puts("");
	s = shell("(map (lambda (x) (plus x 1)) (quote (1 2 3)))", &end);
	print_symbol(eval(s));
	puts("test");
	s = shell("(concat (quote (1 2)) (quote (3 4)))", &end);
	print_symbol(eval(s));
	puts("test");
	gc();
	
	
#endif
#if 0
	for (i = 0; i < 100; i++){
		s = shell("(plus 1 2)", &end);
		print_symbol(eval(s));
		puts("");
		printf("cons=%d\n", cons_heap_ptr);
	}
	gc();
	puts("01");
	dump_symbol();
	printf("cons=%d\n", cons_heap_ptr);
	s = shell("(sum 3)", &end);
	print_symbol(eval(s));
	puts("");
#endif
	while (fgets(str, 256, stdin) != NULL){
		i = 0;
		while (str[i] != '\0'){
			if (str[i] == '\n'){
				str[i] = '\0';
			}
			i++;
		}
		s = shell(str, &end);
		print_symbol(s);
		print_symbol(eval(s));
		puts("");
	}
	return (0);
}

