## 字符常量表示为第一个字符的地址；
## c语言函数A调用函数B，一定要将B定义在A的前面（醉醉的）
## "control may reach end of non-void function"这个报错是因为：
goto err: 里没有返回值；

## 标点符号：
`！ " # $ % & ' () * + , - . / : ; < = > ? @ [ \ ] ^ _ { | } ~ ` 
## 指针作为函数参数：
如果要修改参数，就要传递参数的指针；
如：
``` c
#include <stdio.h>
#include <stdlib.h>

typedef struct Node Node;

struct Node {
    int data;
    Node *next;
};

void init_list(Node *head, int data) {
    head = (Node*)malloc(sizeof(Node));
    if(head == NULL) {
        perror("memory allocation error!");
        return ;
    }

    head->data = data;
    head->next = NULL;
}

int main() {
    Node *head = NULL;
    int N;
    printf("N:");
    scanf("%d", &N);
    init_list(head, N);
    printf("%d\n", head->data);

    return 0;
}
```
`init_list`这函数没能改变head值，由于参数传递的是head的副本；
要穿参数的指针；
``` c
void init_list(Node **head, int data) {
    *head = malloc(sizeof(Node));
    (*head)->data = data;
    (*head)->next = NULL;
}
init_list(&head, N);
```

## 指针数组：
``` c
short a[] = {1, 3, 4};
char s[4] = "abc";
```
注意第二种写法，char类型的数组，都会在最后加一个'\0'的结束符，所以是4不是3